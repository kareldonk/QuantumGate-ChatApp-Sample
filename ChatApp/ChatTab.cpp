#include "pch.h"
#include "ChatTab.h"
#include "MainPage.h"

namespace winrt::ChatApp::implementation
{
	ChatTab::ChatTab(const QuantumGate::PeerLUID pluid, ChatExtender* extender, const std::wstring& header,
					 MainPage* main_page, const winrt::Windows::UI::Xaml::Controls::PivotItem root) :
		m_MainPage(main_page), m_PeerLUID(pluid), m_Extender(extender), m_PivotItem(root)
	{
		SetHeader(header);

		// Save the peer LUID in the tag of the PivotItem control
		root.Tag(winrt::box_value(ChatExtender::LUIDToWstring(pluid).c_str()));

		// Save pointers to all the controls for quick access
		m_ChatMessageView = GetChild(L"ChatMessageView", m_PivotItem).as<winrt::Windows::UI::Xaml::Controls::ScrollViewer>();
		m_ChatMessageContainer = GetChild(L"ChatMessageContainer", m_PivotItem).as<winrt::Windows::UI::Xaml::Controls::ItemsControl>();
		m_ChatMessageBox = GetChild(L"ChatMessageBox", m_PivotItem).as<winrt::Windows::UI::Xaml::Controls::TextBox>();
		m_CloseButton = GetChild(L"CloseButton", m_PivotItem).as<winrt::Windows::UI::Xaml::Controls::Button>();
		m_SendButton = GetChild(L"ChatMessageSendButton", m_PivotItem).as<winrt::Windows::UI::Xaml::Controls::Button>();

		m_ChatMessageBox.KeyUp({ this, &ChatTab::ChatMessageBoxKeyUp });
		m_SendButton.Click({ this, &ChatTab::SendButtonClicked });

		// Peer LUID of 0 is used for the broadcast tab
		if (pluid == 0)
		{
			// Broadcast tab does not get a close button
			m_CloseButton.Visibility(winrt::Windows::UI::Xaml::Visibility::Collapsed);

			AddInformationBox(L"This is the broadcast tab. Broadcast messages sent by peers will be displayed here. "
							  "Any messages sent from this tab will be sent to all peers.");
		}
		else
		{
			m_CloseButton.Click({ this, &ChatTab::CloseButtonClicked });

			AddInformationBox(L"This is a private messaging tab. Private messages sent by " + header + L" will be displayed here. "
							  "Any messages sent from this tab will only be sent to " + header + L".");
		}
	}

	winrt::Windows::UI::Xaml::DependencyObject ChatTab::GetChild(const winrt::hstring& name,
																 const winrt::Windows::UI::Xaml::DependencyObject& root_dp)
	{
		// Use of VisualTreeHelper is recommended because it also searches in control templates
		const auto num_children = winrt::Windows::UI::Xaml::Media::VisualTreeHelper::GetChildrenCount(root_dp);
		for (int i = 0; i < num_children; ++i)
		{
			const auto current = winrt::Windows::UI::Xaml::Media::VisualTreeHelper::GetChild(root_dp, i);

			const auto n = current.as<winrt::Windows::UI::Xaml::FrameworkElement>().Name();
			if (n == name) return current;

			auto result = GetChild(name, current);
			if (result != nullptr) return result;
		}

		return nullptr;
	}

	void ChatTab::FocusOnChatMessageBox()
	{
		m_ChatMessageBox.Focus(winrt::Windows::UI::Xaml::FocusState::Keyboard);
	}

	void ChatTab::ScrollToLastChatMessage()
	{
		m_ChatMessageContainer.UpdateLayout();
		m_ChatMessageView.ChangeView(nullptr, m_ChatMessageView.ScrollableHeight(), nullptr);
	}

	void ChatTab::SetHeader(const std::wstring& txt)
	{
		if (m_Header != txt)
		{
			m_PivotItem.Header(winrt::box_value(txt.c_str()));
			m_Header = txt;
		}
	}

	void ChatTab::AddToChatMessageContainer(const winrt::Windows::Foundation::IInspectable& item)
	{
		m_ChatMessageContainer.Items().Append(item);
		ScrollToLastChatMessage();
	}

	void ChatTab::AddInformationBox(const std::wstring& info)
	{
		winrt::Windows::UI::Xaml::Controls::Border border;
		border.Style(GetResources().Lookup(winrt::box_value(L"InfoBox")).as<winrt::Windows::UI::Xaml::Style>());

		winrt::Windows::UI::Xaml::Controls::TextBlock textblock;
		textblock.Style(GetResources().Lookup(winrt::box_value(L"InfoBoxText")).as<winrt::Windows::UI::Xaml::Style>());

		winrt::Windows::UI::Xaml::Documents::Run labelrun;
		labelrun.Text(L"Note: ");
		labelrun.FontWeight(winrt::Windows::UI::Text::FontWeights::Bold());

		winrt::Windows::UI::Xaml::Documents::Run msgrun;
		msgrun.Text(info);
		textblock.Inlines().Append(labelrun);
		textblock.Inlines().Append(msgrun);

		border.Child(textblock);

		AddToChatMessageContainer(border);
	}

	void ChatTab::AddJoinChatMessage(const std::wstring& pluid)
	{
		winrt::Windows::UI::Xaml::Controls::Border border;
		border.Style(GetResources().Lookup(winrt::box_value(L"PeerJoin")).as<winrt::Windows::UI::Xaml::Style>());

		winrt::Windows::UI::Xaml::Controls::TextBlock textblock;
		textblock.Style(GetResources().Lookup(winrt::box_value(L"PeerJoinText")).as<winrt::Windows::UI::Xaml::Style>());
		textblock.Text(L"Peer " + pluid + L" joined the chat");

		border.Child(textblock);

		AddToChatMessageContainer(border);
	}

	void ChatTab::AddLeaveChatMessage(const std::wstring&, const std::wstring& nickname)
	{
		winrt::Windows::UI::Xaml::Controls::Border border;
		border.Style(GetResources().Lookup(winrt::box_value(L"PeerLeave")).as<winrt::Windows::UI::Xaml::Style>());

		winrt::Windows::UI::Xaml::Controls::TextBlock textblock;
		textblock.Style(GetResources().Lookup(winrt::box_value(L"PeerLeaveText")).as<winrt::Windows::UI::Xaml::Style>());
		textblock.Text(nickname + L" left the chat");

		border.Child(textblock);

		AddToChatMessageContainer(border);
	}

	void ChatTab::AddNicknameChangeChatMessage(const std::wstring& old_nickname, const std::wstring& new_nickname)
	{
		winrt::Windows::UI::Xaml::Controls::Border border;
		border.Style(GetResources().Lookup(winrt::box_value(L"PeerNicknameChange")).as<winrt::Windows::UI::Xaml::Style>());

		winrt::Windows::UI::Xaml::Controls::TextBlock textblock;
		textblock.Style(GetResources().Lookup(winrt::box_value(L"PeerNicknameChangeText")).as<winrt::Windows::UI::Xaml::Style>());
		textblock.Text(old_nickname + L" is now known as " + new_nickname);

		border.Child(textblock);

		AddToChatMessageContainer(border);

		if (m_PeerLUID != 0)
		{
			SetHeader(new_nickname);
		}
	}

	void ChatTab::AddChatMessage(const std::wstring& nickname, const std::wstring& message, const bool own)
	{
		winrt::Windows::UI::Xaml::Controls::TextBlock textblock;
		winrt::Windows::UI::Xaml::Style style;
		textblock.Style(GetResources().Lookup(winrt::box_value(own ? L"OwnChatText" : L"ChatText")).as<winrt::Windows::UI::Xaml::Style>());

		winrt::Windows::UI::Xaml::Documents::Run nickrun;
		nickrun.Text(nickname + L": ");
		nickrun.FontWeight(winrt::Windows::UI::Text::FontWeights::SemiBold());

		winrt::Windows::UI::Xaml::Documents::Run msgrun;
		msgrun.Text(message);
		textblock.Inlines().Append(nickrun);
		textblock.Inlines().Append(msgrun);

		AddToChatMessageContainer(textblock);
	}

	void ChatTab::SendMessage()
	{
		if (m_ChatMessageBox.Text().size() > 0)
		{
			const std::wstring msg = m_ChatMessageBox.Text().c_str();
			auto ret{ false };

			if (m_PeerLUID == 0)
			{
				ret = m_Extender->BroadcastChatMessage(msg);
			}
			else
			{
				// Private message to a specific peer
				ret = m_Extender->SendChatMessage(m_PeerLUID, msg);
			}

			if (ret)
			{
				AddChatMessage(L"You", msg, true);
				m_ChatMessageBox.Text(L"");
			}
		}
	}

	void ChatTab::ChatMessageBoxKeyUp(const winrt::Windows::Foundation::IInspectable&,
									  const winrt::Windows::UI::Xaml::Input::KeyRoutedEventArgs& e)
	{
		// If Enter key is pressed in the chat textbox we send the message
		if (e.Key() == winrt::Windows::System::VirtualKey::Enter)
		{
			SendMessage();
		}
	}

	void ChatTab::CloseButtonClicked(const winrt::Windows::Foundation::IInspectable&,
									 const winrt::Windows::UI::Xaml::RoutedEventArgs&)
	{
		// Main page will handle closing this tab
		m_MainPage->CloseChatTab(m_PeerLUID);
	}

	void ChatTab::SendButtonClicked(const winrt::Windows::Foundation::IInspectable&,
									const winrt::Windows::UI::Xaml::RoutedEventArgs&)
	{
		SendMessage();
	}
}