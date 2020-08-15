#pragma once

#include <unordered_map>
#include <shared_mutex>

class ChatExtender final : public QuantumGate::Extender
{
	struct PeerData final
	{
		QuantumGate::Peer Peer;
		std::wstring Nickname;
	};

	using PeerContainer = std::unordered_map<QuantumGate::PeerLUID, PeerData>;

	enum class MessageType : std::uint8_t
	{
		Unknown = 0,
		NicknameChange,
		PrivateChatMessage,
		BroadcastChatMessage
	};

public:
	using PeerConnectCallbackType = QuantumGate::Callback<void(const QuantumGate::Peer&)>;
	using PeerDisconnectCallbackType = QuantumGate::Callback<void(const QuantumGate::Peer&, const std::wstring&)>;
	using PeerNicknameChangeCallbackType = QuantumGate::Callback<void(const QuantumGate::Peer&, const std::wstring&, const std::wstring&)>;
	using PeerChatMessageCallbackType = QuantumGate::Callback<void(const QuantumGate::Peer&, const std::wstring&, const std::wstring&, const bool)>;

	constexpr static std::size_t MaxNicknameLength{ 30 }; // Number of characters (not bytes)
	constexpr static std::size_t MaxChatMessageLength{ 1024 }; // Number of characters (not bytes)

public:
	ChatExtender();
	virtual ~ChatExtender() = default;

	void SetNickname(const wchar_t* nickname);
	bool SendChatMessage(const QuantumGate::PeerLUID pluid, const std::wstring& msg);
	bool BroadcastChatMessage(const std::wstring& message);

	// Callbacks for updating the main window
	void OnPeerConnect(PeerConnectCallbackType&& cb) noexcept { m_PeerConnectCallback = std::move(cb); }
	void OnPeerDisconnect(PeerDisconnectCallbackType&& cb) noexcept { m_PeerDisconnectCallback = std::move(cb); }
	void OnPeerNicknameChanged(PeerNicknameChangeCallbackType&& cb) noexcept { m_PeerNicknameChangeCallback = std::move(cb); }
	void OnPeerChatMessage(PeerChatMessageCallbackType&& cb) noexcept { m_PeerChatMessageCallback = std::move(cb); }

	// Utility functions
	static std::wstring LUIDToWstring(const QuantumGate::PeerLUID pluid) noexcept;
	static QuantumGate::PeerLUID WStringToLUID(const std::wstring& pluid) noexcept;

private:
	// Extender callbacks for QuantumGate
	bool OnStartup();
	void OnPostStartup();
	void OnPreShutdown();
	void OnShutdown();
	void OnPeerEvent(QuantumGate::Extender::PeerEvent&& event);
	QuantumGate::Extender::PeerEvent::Result OnPeerMessage(QuantumGate::Extender::PeerEvent&& event);

	void AddPeer(const QuantumGate::PeerLUID pluid, const QuantumGate::Peer& peer);
	void RemovePeer(const QuantumGate::PeerLUID pluid);

	bool SendNicknameChange(const QuantumGate::PeerLUID pluid) const;
	void BroadcastNicknameChange() const;

	bool SendChatMessage(const QuantumGate::PeerLUID pluid, const std::wstring& msg, const bool isbroadcast);

private:
	PeerContainer m_Peers;
	mutable std::shared_mutex m_PeersMutex;

	std::wstring m_Nickname;
	mutable std::shared_mutex m_NicknameMutex;

	PeerConnectCallbackType m_PeerConnectCallback;
	PeerDisconnectCallbackType m_PeerDisconnectCallback;
	PeerNicknameChangeCallbackType m_PeerNicknameChangeCallback;
	PeerChatMessageCallbackType m_PeerChatMessageCallback;
};

