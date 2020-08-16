#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"

namespace winrt::ChatApp::implementation
{
	MainPage::MainPage()
	{
		InitializeComponent();
	}

	int32_t MainPage::MyProperty()
	{
		throw hresult_not_implemented();
	}

	void MainPage::MyProperty(int32_t /* value */)
	{
		throw hresult_not_implemented();
	}

	void MainPage::ShowMessage(const winrt::hstring& title, const winrt::hstring& msg) noexcept
	{
		Windows::UI::Popups::MessageDialog dlg(msg, title);
		Windows::UI::Popups::UICommand cmd(L"Close");
		dlg.Commands().Append(cmd);

		// Set the command that will be invoked when pressing Esc
		dlg.CancelCommandIndex(0);

		// Show the message dialog
		dlg.ShowAsync();
	}

	void MainPage::ShowErrorMessage(const winrt::hstring& msg) noexcept
	{
		ShowMessage(L"An error occured", msg);
	}

	void MainPage::EnableAllChildControls(const winrt::Windows::Foundation::IInspectable& element, const bool enable) noexcept
	{
		try
		{
			element.as<winrt::Windows::UI::Xaml::Controls::Control>().IsEnabled(enable);
		}
		catch (...) {}

		try
		{
			const auto children = element.as<winrt::Windows::UI::Xaml::FrameworkElement>().GetChildrenInTabFocusOrder();
			if (children != nullptr)
			{
				for (const auto& c : children)
				{
					EnableAllChildControls(c, enable);
				}
			}
		}
		catch (...) {}
	}

	void MainPage::ConnectToPeer() noexcept
	{
		QuantumGate::IPAddress ip;
		if (!QuantumGate::IPAddress::TryParse(IPAddressTextBox().Text().c_str(), ip))
		{
			ShowErrorMessage(L"Invalid peer IP address specified.");
			return;
		}

		wchar_t* end{ nullptr };
		const auto port = static_cast<std::uint16_t>(std::wcstol(PortTextBox().Text().c_str(), &end, 10));
		if (!(port > 0 && port < (std::numeric_limits<std::uint16_t>::max)()))
		{
			ShowErrorMessage(L"Invalid peer port number specified.");
			return;
		}

		SetConnectTabStateConnecting();

		// Local instance needs to be online first
		if (!m_Local.IsRunning())
		{
			if (!StartLocalInstance())
			{
				SetConnectTabStateReady();
				return;
			}
		}

		QuantumGate::ConnectParameters cp;
		cp.PeerIPEndpoint = QuantumGate::IPEndpoint(ip, port);

		// This overload of the ConnectTo function will connect
		// asynchronously; we pass along a callback function which will get
		// executed once a connection is established or has failed.
		const auto result = m_Local.ConnectTo(std::move(cp), [&](QuantumGate::PeerLUID,
																 QuantumGate::Result<QuantumGate::Peer> result) mutable
		{
			// Since we will be updating the UI from within this callback function,
			// and the callback function will be called by one of the QuantumGate threads, 
			// we'll have to pass execution on to the UI thread. Windows does not
			// allow updating the UI from another thread except for the one where
			// it was created (deadlocks or crashes happen). This pattern is used
			// below in the code as well.
			this->Dispatcher().RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
										[&, r = std::move(result)]()
			{
				SetConnectTabStateReady();

				if (r.Failed())
				{
					std::wstring str{ L"Failed to connect to peer (" + r.GetErrorString() + L")." };
					ShowErrorMessage(str.c_str());
				}
				else ConnectionPivot().SelectedIndex(0);
			});
		});

		if (result.Succeeded())
		{
			if (result.GetValue().second)
			{
				ShowMessage(L"New connection", L"A connection to the peer already existed.");

				SetConnectTabStateReady();

				ConnectionPivot().SelectedIndex(0);
			}
		}
		else
		{
			std::wstring str{ L"Failed to connect to the peer (" + result.GetErrorString() + L")." };
			ShowErrorMessage(str.c_str());
		}
	}

	void MainPage::SetConnectTabStateReady() noexcept
	{
		ConnectProgressRing().IsActive(false);
		ConnectButton().Content(winrt::box_value(L"Connect"));
		EnableAllChildControls(ConnectSettingsPanel(), true);
	}

	void MainPage::SetConnectTabStateConnecting() noexcept
	{
		// Disable controls while we connect
		EnableAllChildControls(ConnectSettingsPanel(), false);
		ConnectButton().Content(winrt::box_value(L"Connecting..."));
		ConnectProgressRing().IsActive(true);
	}

	ChatTab* MainPage::AddChatTab(const QuantumGate::PeerLUID pluid, const std::wstring title)
	{
		winrt::Windows::UI::Xaml::Controls::PivotItem itm;

		// Apply a template defined in the App.xaml to the control (the template 
		// has all the needed child controls)
		itm.Template(winrt::Windows::UI::Xaml::Application::Current().Resources()
					 .Lookup(winrt::box_value(L"ChatWindow")).as<winrt::Windows::UI::Xaml::Controls::ControlTemplate>());
		
		ChatPivot().Items().Append(itm);
		ChatPivot().SelectedItem(itm);
		ChatPivot().UpdateLayout();

		std::unique_lock lock(m_ChatTabsMutex);

		const auto it = m_ChatTabs.emplace(pluid, std::make_unique<ChatTab>(pluid, m_Extender.get(), title, this, itm));
		if (it.second)
		{
			it.first->second->FocusOnChatMessageBox();
			return it.first->second.get();
		}

		return nullptr;
	}

	void MainPage::CloseChatTab(const QuantumGate::PeerLUID pluid)
	{
		this->Dispatcher().RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
									[&, spluid = pluid]()
		{
			std::unique_lock lock(m_ChatTabsMutex);

			for (const auto& tab : m_ChatTabs)
			{
				if (tab.second->GetPeerLUID() == spluid)
				{
					std::uint32_t idx{ 0 };
					if (ChatPivot().Items().IndexOf(tab.second->GetPivotItem(), idx))
					{
						ChatPivot().Items().RemoveAt(idx);
					}

					m_ChatTabs.erase(spluid);

					return;
				}
			}
		});
	}

	void MainPage::ConnectButtonClicked(const winrt::Windows::Foundation::IInspectable&,
										const winrt::Windows::UI::Xaml::RoutedEventArgs&)
	{
		ConnectToPeer();
	}

	bool MainPage::InitializeChatExtender() noexcept
	{
		if (m_Extender != nullptr)
		{
			// Already initialized
			return true;
		}

		try
		{
			// Make new object
			m_Extender = std::make_shared<ChatExtender>();

			// Set nickname
			UpdateExtenderNickname();

			// In the below block we set our custom defined callbacks so that we can
			// get notified of new events by the extender in order to update the UI and do other stuff
			{
				m_Extender->OnPeerConnect([&](const QuantumGate::Peer& peer) mutable
				{
					this->Dispatcher().RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
												[&, speer = peer]()
					{
						const auto pluid = ChatExtender::LUIDToWstring(speer.GetLUID());

						// Add a new peer to the connection list
						{
							winrt::Windows::UI::Xaml::Controls::TextBlock tb;

							// Initially use the peer LUID to identify the peer
							// in the list (will get updated later with the nickname
							tb.Text(pluid.c_str());

							// Also save the peer LUID in the control tag for later use
							tb.Tag(winrt::box_value(pluid.c_str()));

							ConnectionList().Items().Append(tb);
						}

						// Add mesages to the chat tabs
						{
							std::unique_lock lock(m_ChatTabsMutex);

							m_BroadCastChatTab->AddJoinChatMessage(pluid);

							// If we had a private tab open for this peer
							// we also update that one
							const auto it = m_ChatTabs.find(speer.GetLUID());
							if (it != m_ChatTabs.end())
							{
								it->second->AddJoinChatMessage(pluid);
							}
						}
					});
				});

				m_Extender->OnPeerDisconnect([&](const QuantumGate::Peer& peer, const std::wstring& nickname) mutable
				{
					this->Dispatcher().RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
												[&, speer = peer, snickname = nickname]()
					{
						// Remove peer from connection list
						{
							int idx{ -1 };

							for (const auto& itm : ConnectionList().Items())
							{
								++idx;

								const auto tb = itm.as<winrt::Windows::UI::Xaml::Controls::TextBlock>();
								const auto pluid_str = winrt::unbox_value<winrt::hstring>(tb.Tag());
								const auto pluid = ChatExtender::WStringToLUID(pluid_str.c_str());

								if (pluid == speer.GetLUID())
								{
									break;
								}
							}

							if (idx >= 0)
							{
								ConnectionList().Items().RemoveAt(idx);
							}
						}

						const auto pluid_str = ChatExtender::LUIDToWstring(speer.GetLUID());

						// Add mesages to the chat tabs
						{
							std::unique_lock lock(m_ChatTabsMutex);

							m_BroadCastChatTab->AddLeaveChatMessage(pluid_str, snickname);

							// If we had a private tab open for this peer
							// we also update that one
							const auto it = m_ChatTabs.find(speer.GetLUID());
							if (it != m_ChatTabs.end())
							{
								it->second->AddLeaveChatMessage(pluid_str, snickname);
							}
						}
					});
				});

				m_Extender->OnPeerNicknameChanged([&](const QuantumGate::Peer& peer, const std::wstring& nickname,
													  const std::wstring& old_nickname) mutable
				{
					this->Dispatcher().RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
												[&, speer = peer, snickname = nickname, sonickname = old_nickname]()
					{
						// Update peer nickname in connection list
						{
							for (const auto& itm : ConnectionList().Items())
							{
								const auto tb = itm.as<winrt::Windows::UI::Xaml::Controls::TextBlock>();
								const auto pluid_str = winrt::unbox_value<winrt::hstring>(tb.Tag());
								const auto pluid = ChatExtender::WStringToLUID(pluid_str.c_str());

								if (pluid == speer.GetLUID())
								{
									tb.Text(snickname.c_str());
									break;
								}
							}
						}

						// Add mesages to the chat tabs
						{
							std::unique_lock lock(m_ChatTabsMutex);

							m_BroadCastChatTab->AddNicknameChangeChatMessage(sonickname, snickname);

							// If we had a private tab open for this peer
							// we also update that one
							const auto it = m_ChatTabs.find(speer.GetLUID());
							if (it != m_ChatTabs.end())
							{
								it->second->AddNicknameChangeChatMessage(sonickname, snickname);
							}
						}
					});
				});

				m_Extender->OnPeerChatMessage([&](const QuantumGate::Peer& peer, const std::wstring& nickname,
												  const std::wstring& message, const bool isbroadcast) mutable
				{
					this->Dispatcher().RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
												[&, speer = peer, snickname = nickname, smessage = message,
												sisbroadcast = isbroadcast]()
					{
						if (sisbroadcast)
						{
							std::unique_lock lock(m_ChatTabsMutex);
							m_BroadCastChatTab->AddChatMessage(nickname, smessage, false);
						}
						else
						{
							ChatTab* tab{ nullptr };

							// Since we got a private message, check to see if we
							// already had a tab open for that peer
							{
								std::shared_lock lock(m_ChatTabsMutex);
								const auto it = m_ChatTabs.find(speer.GetLUID());
								if (it != m_ChatTabs.end())
								{
									tab = it->second.get();
								}
							}

							if (!tab)
							{
								// We did not have a tab open for that peer so we add one
								tab = AddChatTab(speer.GetLUID(), nickname);
							}

							if (tab)
							{
								std::unique_lock lock(m_ChatTabsMutex);
								tab->AddChatMessage(nickname, smessage, false);
							}
						}
					});
				});
			}

			return true;
		}
		catch (...) {}

		m_Extender.reset();

		ShowErrorMessage(L"Failed to initialize the ChatExtender.");

		return false;
	}

	void MainPage::UpdateExtenderNickname()
	{
		if (m_Extender) m_Extender->SetNickname(NickNameTextBox().Text().c_str());
	}

	bool MainPage::StartLocalInstance() noexcept
	{
		// Make sure our chat extender is ready to go
		if (!InitializeChatExtender()) return false;

		QuantumGate::StartupParameters params;

		// Create a UUID for the local instance with matching keypair;
		// normally you should do this once and save and reload the UUID
		// and keys. The UUID and public key can be shared with other peers,
		// while the private key should be protected and kept private.
		{
			auto [success, uuid, keys] = QuantumGate::UUID::Create(QuantumGate::UUID::Type::Peer,
																   QuantumGate::UUID::SignAlgorithm::EDDSA_ED25519);
			if (success)
			{
				params.UUID = uuid;
				params.Keys = std::move(*keys);
			}
			else
			{
				ShowErrorMessage(L"Failed to create peer UUID.");
				return false;
			}
		}

		// Set the supported algorithms
		params.SupportedAlgorithms.Hash = {
			QuantumGate::Algorithm::Hash::BLAKE2B512
		};
		params.SupportedAlgorithms.PrimaryAsymmetric = {
			QuantumGate::Algorithm::Asymmetric::ECDH_X25519
		};
		params.SupportedAlgorithms.SecondaryAsymmetric = {
			QuantumGate::Algorithm::Asymmetric::KEM_NTRUPRIME
		};
		params.SupportedAlgorithms.Symmetric = {
			QuantumGate::Algorithm::Symmetric::CHACHA20_POLY1305
		};
		params.SupportedAlgorithms.Compression = {
			QuantumGate::Algorithm::Compression::ZSTANDARD
		};

		// Listen for incoming connections on startup
		params.Listeners.Enable = true;

		// Listen for incoming connections on these ports
		params.Listeners.TCPPorts = { 999 };

		// Start extenders on startup
		params.EnableExtenders = true;

		// For our purposes we disable authentication requirement; when
		// authentication is required we would need to add peers to the instance
		// via QuantumGate::Local::GetAccessManager().AddPeer() including their
		// UUID and public key so that they can be authenticated when connecting
		params.RequireAuthentication = false;

		// For our purposes we allow access by default
		m_Local.GetAccessManager().SetPeerAccessDefault(QuantumGate::Access::PeerAccessDefault::Allowed);

		// For our purposes we allow all IP addresses to connect;
		// by default all IP Addresses are blocked
		if (!m_Local.GetAccessManager().AddIPFilter(L"0.0.0.0/0", QuantumGate::Access::IPFilterType::Allowed) ||
			!m_Local.GetAccessManager().AddIPFilter(L"::/0", QuantumGate::Access::IPFilterType::Allowed))
		{
			ShowErrorMessage(L"Failed to add an IP filter.");
			return false;
		}

		// Add our chat extender to the local instance
		if (const auto result = m_Local.AddExtender(m_Extender); result.Failed())
		{
			ShowErrorMessage(L"Failed to add the ChatExtender to the QuantumGate local instance.");
			return false;
		}

		// Start the local instance
		const auto result = m_Local.Startup(params);
		if (result.Failed())
		{
			std::wstring str{ L"Failed to start the QuantumGate local instance (" + result.GetErrorString() + L")." };
			ShowErrorMessage(str.c_str());
			return false;
		}

		Windows::UI::Xaml::Media::SolidColorBrush brush(Windows::UI::Colors::Green());
		StatusText().Foreground(brush);
		StatusText().Text(L"Online");

		OnlineButton().Content(winrt::box_value(L"Go Offline"));
		OnlineButton().Tag(box_value(L"online"));

		m_BroadCastChatTab = AddChatTab(0, L"Broadcast");

		return true;
	}

	void MainPage::ShutdownLocalInstance() noexcept
	{
		m_Local.Shutdown();

		m_Local.GetAccessManager().RemoveAllIPFilters();
		m_Local.RemoveExtender(m_Extender);

		ConnectionList().Items().Clear();

		// Close and remove all tabs
		{
			std::unique_lock lock(m_ChatTabsMutex);

			for (const auto& tab : m_ChatTabs)
			{
				std::uint32_t idx{ 0 };
				if (ChatPivot().Items().IndexOf(tab.second->GetPivotItem(), idx))
				{
					ChatPivot().Items().RemoveAt(idx);
				}
			}

			m_ChatTabs.clear();

			m_BroadCastChatTab = nullptr;
		}

		SetConnectTabStateReady();

		Windows::UI::Xaml::Media::SolidColorBrush brush(Windows::UI::Colors::Black());
		StatusText().Foreground(brush);
		StatusText().Text(L"Offline");

		OnlineButton().Tag(box_value(L"offline"));
		OnlineButton().Content(box_value(L"Go Online"));
	}

	void MainPage::ShowConsoleButtonClicked(const winrt::Windows::Foundation::IInspectable&,
											const winrt::Windows::UI::Xaml::RoutedEventArgs&)
	{
		if (ConsoleButton().Tag().as<hstring>() == L"closed")
		{
			QuantumGate::Console::SetOutput(std::make_shared<QuantumGate::Console::WindowOutput>(false, false));
			QuantumGate::Console::SetVerbosity(QuantumGate::Console::Verbosity::Debug);

			ConsoleButton().Tag(box_value(L"open"));
			ConsoleButton().Content(box_value(L"Close Console"));
		}
		else
		{
			QuantumGate::Console::SetOutput(nullptr);
			ConsoleButton().Tag(box_value(L"closed"));
			ConsoleButton().Content(box_value(L"Show Console"));
		}
	}

	void MainPage::DisconnectButtonClicked(const winrt::Windows::Foundation::IInspectable&,
										   const winrt::Windows::UI::Xaml::RoutedEventArgs&)
	{
		const auto idx = ConnectionList().SelectedIndex();
		if (idx >= 0)
		{
			const auto tb = ConnectionList().Items().GetAt(idx).as<winrt::Windows::UI::Xaml::Controls::TextBlock>();

			// Get the peer LUID of the selected peer from the element tag
			const auto pluid_str = winrt::unbox_value<winrt::hstring>(tb.Tag());
			const auto pluid = ChatExtender::WStringToLUID(pluid_str.c_str());

			const auto result = m_Local.DisconnectFrom(pluid);
			if (result.Failed())
			{
				std::wstring str{ L"Failed to disconnect peer (" + result.GetErrorString() + L")." };
				ShowErrorMessage(str.c_str());
			}
		}
	}

	void MainPage::ConnectionListSelectionChanged(const winrt::Windows::Foundation::IInspectable&,
												  const winrt::Windows::UI::Xaml::RoutedEventArgs&)
	{
		if (ConnectionList().SelectedIndex() >= 0)
		{
			PrivateChatButton().IsEnabled(true);
			DisconnectButton().IsEnabled(true);
		}
		else
		{
			PrivateChatButton().IsEnabled(false);
			DisconnectButton().IsEnabled(false);
		}
	}

	void MainPage::NickNameTextBoxLostFocus(const winrt::Windows::Foundation::IInspectable&,
											const winrt::Windows::UI::Xaml::RoutedEventArgs&)
	{
		UpdateExtenderNickname();
	}

	void MainPage::NickNameTextBoxKeyUp(const winrt::Windows::Foundation::IInspectable&,
										const winrt::Windows::UI::Xaml::Input::KeyRoutedEventArgs& e)
	{
		if (e.Key() == winrt::Windows::System::VirtualKey::Enter)
		{
			UpdateExtenderNickname();
		}
	}

	void MainPage::PrivateChatButtonClicked(const winrt::Windows::Foundation::IInspectable&,
											const winrt::Windows::UI::Xaml::RoutedEventArgs&)
	{
		const auto idx = ConnectionList().SelectedIndex();
		if (idx >= 0)
		{
			const auto tb = ConnectionList().Items().GetAt(idx).as<winrt::Windows::UI::Xaml::Controls::TextBlock>();
			const auto pluid_str = winrt::unbox_value<winrt::hstring>(tb.Tag());
			const auto pluid = ChatExtender::WStringToLUID(pluid_str.c_str());

			{
				std::shared_lock lock(m_ChatTabsMutex);

				// First check if we already had a tab open for this peer,
				// and if so we just bring it into focus
				for (const auto& tab : m_ChatTabs)
				{
					if (tab.second->GetPeerLUID() == pluid)
					{
						ChatPivot().SelectedItem(tab.second->GetPivotItem());
						tab.second->FocusOnChatMessageBox();
						return;
					}
				}
			}

			// If we get here there was no tab open for the peer
			// and we need to add a new one
			AddChatTab(pluid, tb.Text().c_str());
		}
	}

	void MainPage::OnlineButtonClicked(const winrt::Windows::Foundation::IInspectable&,
									   const winrt::Windows::UI::Xaml::RoutedEventArgs&)
	{
		if (OnlineButton().Tag().as<hstring>() == L"offline")
		{
			StartLocalInstance();
		}
		else
		{
			ShutdownLocalInstance();
		}
	}
}
