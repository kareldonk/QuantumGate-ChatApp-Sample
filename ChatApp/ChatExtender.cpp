#include "pch.h"
#include "ChatExtender.h"

ChatExtender::ChatExtender() :
	QuantumGate::Extender(QuantumGate::ExtenderUUID(L"c055850e-2a88-f990-4e58-ad915552a375"),
						  QuantumGate::String(L"Chat Extender"))
{
	// Add the callback functions for this extender; this can also be done
	// in another function instead of the constructor, as long as you set the callbacks
	// before adding the extender to the local instance
	if (!SetStartupCallback(QuantumGate::MakeCallback(this, &ChatExtender::OnStartup)) ||
		!SetPostStartupCallback(QuantumGate::MakeCallback(this, &ChatExtender::OnPostStartup)) ||
		!SetPreShutdownCallback(QuantumGate::MakeCallback(this, &ChatExtender::OnPreShutdown)) ||
		!SetShutdownCallback(QuantumGate::MakeCallback(this, &ChatExtender::OnShutdown)) ||
		!SetPeerEventCallback(QuantumGate::MakeCallback(this, &ChatExtender::OnPeerEvent)) ||
		!SetPeerMessageCallback(QuantumGate::MakeCallback(this, &ChatExtender::OnPeerMessage)))
	{
		throw std::exception("Failed to set one or more extender callbacks");
	}
}

bool ChatExtender::OnStartup()
{
	// This function gets called by the QuantumGate instance to notify
	// an extender to initialize and startup

	// Return true if initialization was successful, otherwise return false and
	// QuantumGate won't be sending this extender any notifications
	return true;
}

void ChatExtender::OnPostStartup()
{
	// This function gets called by the QuantumGate instance to notify
	// an extender of the fact that the startup procedure for this extender has
	// been completed successfully and the extender can now interact with the instance

}

void ChatExtender::OnPreShutdown()
{
	// This callback function gets called by the QuantumGate instance to notify
	// an extender that the shut down procedure has been initiated for this extender.
	// The extender should stop all activity and prepare for deinitialization before
	// returning from this function

}

void ChatExtender::OnShutdown()
{
	// This callback function gets called by the QuantumGate instance to notify an
	// extender that it has been shut down completely and should now deinitialize and
	// free resources

}

void ChatExtender::OnPeerEvent(QuantumGate::Extender::PeerEvent&& event)
{
	// This callback function gets called by the QuantumGate instance to notify an
	// extender of a peer event

	switch (event.GetType())
	{
		case QuantumGate::Extender::PeerEvent::Type::Connected:
		{
			const auto result = event.GetPeer();
			if (result.Succeeded())
			{
				AddPeer(event.GetPeerLUID(), result.GetValue());

				// As soon as a new peer connects we send them our nickname
				// as the very first message
				SendNicknameChange(event.GetPeerLUID());
			}
			break;
		}
		case QuantumGate::Extender::PeerEvent::Type::Disconnected:
		{
			RemovePeer(event.GetPeerLUID());
		}
		default:
		{
			break;
		}
	}
}

QuantumGate::Extender::PeerEvent::Result ChatExtender::OnPeerMessage(QuantumGate::Extender::PeerEvent&& event)
{
	// This callback function gets called by the QuantumGate instance to notify an
	// extender of a peer message event

	QuantumGate::Extender::PeerEvent::Result result;

	if (event.GetMessageData() != nullptr)
	{
		QuantumGate::BufferView msg_data_view{ *event.GetMessageData() };

		MessageType msg_type{ MessageType::Unknown };

		// First byte is the message type
		if (msg_data_view.GetSize() > 0)
		{
			msg_type = static_cast<MessageType>(msg_data_view[0]);

			// Remove first byte
			msg_data_view.RemoveFirst(1);
		}

		switch (msg_type)
		{
			case MessageType::NicknameChange:
			{
				// Message recognized
				result.Handled = true;

				// The rest of the buffer should be what we expect
				if (msg_data_view.GetSize() == (MaxNicknameLength * sizeof(std::wstring::value_type)))
				{
					std::unique_lock lock(m_PeersMutex);

					// Look for the peer in our collection; the peer should
					// already exist there otherwise something is wrong
					const auto it = m_Peers.find(event.GetPeerLUID());
					if (it != m_Peers.end())
					{
						const auto old_nickname = it->second.Nickname;

						// Copy new nickname
						it->second.Nickname.resize(MaxNicknameLength);
						std::memcpy(it->second.Nickname.data(), msg_data_view.GetBytes(), msg_data_view.GetSize());

						// Message handled successfully
						result.Success = true;

						// Need to update the main window UI with new message
						m_PeerNicknameChangeCallback(it->second.Peer, it->second.Nickname, old_nickname);
					}
				}
				break;
			}
			case MessageType::PrivateChatMessage:
			case MessageType::BroadcastChatMessage:
			{
				// Message recognized
				result.Handled = true;

				// At least sizeof(std::uint16_t) bytes need to be present or there's a problem
				std::uint16_t message_byte_len{ 0 };
				if (msg_data_view.GetSize() > sizeof(message_byte_len))
				{
					// Read message size and check it
					message_byte_len = *reinterpret_cast<const std::uint16_t*>(msg_data_view.GetBytes());
					if (message_byte_len > 0 && message_byte_len <= (MaxChatMessageLength * sizeof(std::wstring::value_type)))
					{
						// Remove message size from buffer
						msg_data_view.RemoveFirst(sizeof(message_byte_len));

						// The rest of the buffer should match the expected size of the message
						if (msg_data_view.GetSize() == message_byte_len)
						{
							// Copy message from buffer
							std::wstring message;
							message.resize(message_byte_len / sizeof(std::wstring::value_type));
							std::memcpy(message.data(), msg_data_view.GetBytes(), message_byte_len);

							std::shared_lock lock(m_PeersMutex);

							// Look for the peer in our collection; the peer should
							// already exist there otherwise something is wrong
							const auto it = m_Peers.find(event.GetPeerLUID());
							if (it != m_Peers.end())
							{
								// Message handled successfully
								result.Success = true;

								const bool isbroadcast{ msg_type == MessageType::BroadcastChatMessage };

								// Need to update the main window UI with new message
								m_PeerChatMessageCallback(it->second.Peer, it->second.Nickname, message, isbroadcast);
							}
						}
					}
				}
				break;
			}
			default:
			{
				assert(false);
				break;
			}
		}
	}

	// If we return false for Handled and Success too often,
	// QuantumGate will disconnect the misbehaving peer eventually
	// as its reputation declines
	return result;
}

std::wstring ChatExtender::LUIDToWstring(const QuantumGate::PeerLUID pluid) noexcept
{
	return std::to_wstring(pluid);
}

QuantumGate::PeerLUID ChatExtender::WStringToLUID(const std::wstring& pluid) noexcept
{
	wchar_t* end{ nullptr };
	return std::wcstoull(pluid.c_str(), &end, 10);
}

void ChatExtender::SetNickname(const wchar_t* nickname)
{
	auto broadcast_change{ false };

	{
		std::unique_lock lock(m_NicknameMutex);

		if (m_Nickname != nickname)
		{
			m_Nickname = nickname;

			if (m_Nickname.size() > MaxNicknameLength)
			{
				m_Nickname.resize(MaxNicknameLength);
			}

			// Only if the extender is in the running state
			if (IsRunning())
			{
				broadcast_change = true;
			}
		}
	}

	if (broadcast_change)
	{
		// Since we changed our nickname we need to let all peers know
		BroadcastNicknameChange();
	}
}

void ChatExtender::AddPeer(const QuantumGate::PeerLUID pluid, const QuantumGate::Peer& peer)
{
	std::unique_lock lock(m_PeersMutex);

	m_Peers.emplace(pluid, PeerData{ peer, LUIDToWstring(pluid) });

	// New peer connected so we need to update the main window UI
	m_PeerConnectCallback(peer);
}

void ChatExtender::RemovePeer(const QuantumGate::PeerLUID pluid)
{
	std::unique_lock lock(m_PeersMutex);

	const auto it = m_Peers.find(pluid);
	if (it != m_Peers.end())
	{
		// Peer disconnected so we need to update the main window UI
		m_PeerDisconnectCallback(it->second.Peer, it->second.Nickname);

		m_Peers.erase(pluid);
	}
}

bool ChatExtender::SendNicknameChange(const QuantumGate::PeerLUID pluid) const
{
	constexpr std::uint8_t msgtype = static_cast<std::uint8_t>(MessageType::NicknameChange);

	QuantumGate::Buffer buffer;

	// wstring uses multiple bytes per character (2 on windows) so 
	// we need to calculate the actual storage size in bytes
	const auto nickname_byte_len = MaxNicknameLength * sizeof(std::wstring::value_type);
	
	// We allocate enough room in the buffer to store:
	// message type, the nickname (which is a fixed length of MaxNicknameLength)
	buffer.Allocate(1 + nickname_byte_len);

	// Copy message type into buffer
	buffer[0] = QuantumGate::Byte{ msgtype };

	{
		std::shared_lock lock(m_NicknameMutex);

		// Copy nickname into buffer
		std::memcpy(buffer.GetBytes() + 1, m_Nickname.data(), m_Nickname.size() * sizeof(std::wstring::value_type));
	}

	QuantumGate::SendParameters params{
		.Compress = true,
		.Priority = QuantumGate::SendParameters::PriorityOption::Normal
	};

	const auto result = SendMessageTo(pluid, std::move(buffer), params);
	// Normally we'd also need to handle failures where we can retry
	// such as buffer full conditions, but we keep it simple here;
	// see the return codes for SendMessageTo in the QuantumGate
	// documentation for more information.
	if (result.Succeeded()) return true;

	return false;
}

void ChatExtender::BroadcastNicknameChange() const
{
	std::shared_lock lock(m_PeersMutex);

	for (const auto& peer : m_Peers)
	{
		SendNicknameChange(peer.first);
	}
}

bool ChatExtender::SendChatMessage(const QuantumGate::PeerLUID pluid, const std::wstring& msg, const bool isbroadcast)
{
	if (msg.size() == 0 || msg.size() > MaxChatMessageLength) return false;

	const std::uint8_t msgtype = isbroadcast ?
		static_cast<std::uint8_t>(MessageType::BroadcastChatMessage) :
		static_cast<std::uint8_t>(MessageType::PrivateChatMessage);

	QuantumGate::Buffer buffer;

	// wstring uses multiple bytes per character (2 on windows) so 
	// we need to calculate the actual storage size in bytes
	const std::uint16_t message_byte_len = static_cast<std::uint16_t>(msg.size() * sizeof(std::wstring::value_type));

	// We allocate enough room in the buffer to store:
	// message type, the message size, and the message itself
	buffer.Allocate(1 + sizeof(message_byte_len) + message_byte_len);

	// Copy message type into buffer
	buffer[0] = QuantumGate::Byte{ msgtype };
	// Copy message size into buffer
	std::memcpy(buffer.GetBytes() + 1, &message_byte_len, sizeof(message_byte_len));
	// Copy the message into the buffer
	std::memcpy(buffer.GetBytes() + 1 + sizeof(message_byte_len), msg.data(), message_byte_len);

	QuantumGate::SendParameters params{
		.Compress = true,
		.Priority = QuantumGate::SendParameters::PriorityOption::Normal
	};

	const auto result = SendMessageTo(pluid, std::move(buffer), params);
	// Normally we'd also need to handle failures where we can retry
	// such as buffer full conditions, but we keep it simple here;
	// see the return codes for SendMessageTo in the QuantumGate
	// documentation for more information.
	if (result.Succeeded()) return true;

	return false;
}

bool ChatExtender::SendChatMessage(const QuantumGate::PeerLUID pluid, const std::wstring& msg)
{
	// This sends a message to a specific peer
	return SendChatMessage(pluid, msg, false);
}

bool ChatExtender::BroadcastChatMessage(const std::wstring& message)
{
	std::shared_lock lock(m_PeersMutex);

	// Broadcast simply sends the same message to all connected peers
	for (const auto& peer : m_Peers)
	{
		SendChatMessage(peer.first, message, true);
	}

	return true;
}
