#pragma once

#include "ChatExtender.h"

namespace winrt::ChatApp::implementation
{
    struct MainPage;

    class ChatTab final
    {
    public:
        ChatTab(const QuantumGate::PeerLUID pluid, ChatExtender* extender, const std::wstring& header,
                MainPage* main_page, const winrt::Windows::UI::Xaml::Controls::PivotItem root);
        ChatTab(const ChatTab&) = delete;
        ChatTab(ChatTab&&) noexcept = delete;
        ~ChatTab() = default;
        ChatTab& operator=(const ChatTab&) = delete;
        ChatTab& operator=(ChatTab&&) noexcept = delete;

        [[nodiscard]] inline QuantumGate::PeerLUID GetPeerLUID() const noexcept { return m_PeerLUID; }
        [[nodiscard]] inline winrt::Windows::UI::Xaml::Controls::PivotItem GetPivotItem() const noexcept { return m_PivotItem; }

        void SetHeader(const std::wstring& txt);

        void AddInformationBox(const std::wstring& info);
        void AddJoinChatMessage(const std::wstring& pluid);
        void AddLeaveChatMessage(const std::wstring& pluid, const std::wstring& nickname);
        void AddNicknameChangeChatMessage(const std::wstring& old_nickname, const std::wstring& new_nickname);
        void AddChatMessage(const std::wstring& nickname, const std::wstring& message, const bool own);

        void FocusOnChatMessageBox();
        void ScrollToLastChatMessage();

    private:
        void AddToChatMessageContainer(const winrt::Windows::Foundation::IInspectable& item);

        winrt::Windows::UI::Xaml::DependencyObject GetChild(const winrt::hstring& name,
                                                            const winrt::Windows::UI::Xaml::DependencyObject& root_dp);

        [[nodiscard]] inline winrt::Windows::UI::Xaml::ResourceDictionary GetResources() const
        {
            return winrt::Windows::UI::Xaml::Application::Current().Resources();
        }

        void SendMessage();

        // Event handlers
        void ChatMessageBoxKeyUp(const winrt::Windows::Foundation::IInspectable& sender,
                                 const winrt::Windows::UI::Xaml::Input::KeyRoutedEventArgs& e);
        void CloseButtonClicked(const winrt::Windows::Foundation::IInspectable& sender,
                                const winrt::Windows::UI::Xaml::RoutedEventArgs& e);
        void SendButtonClicked(const winrt::Windows::Foundation::IInspectable& sender,
                               const winrt::Windows::UI::Xaml::RoutedEventArgs& e);

    private:
        MainPage* const m_MainPage{ nullptr };
        const QuantumGate::PeerLUID m_PeerLUID{ 0 };
        ChatExtender* const m_Extender{ nullptr };
        std::wstring m_Header;
        winrt::Windows::UI::Xaml::Controls::PivotItem m_PivotItem;
        winrt::Windows::UI::Xaml::Controls::ItemsControl m_ChatMessageContainer;
        winrt::Windows::UI::Xaml::Controls::TextBox m_ChatMessageBox;
        winrt::Windows::UI::Xaml::Controls::ScrollViewer m_ChatMessageView;
        winrt::Windows::UI::Xaml::Controls::Button m_CloseButton;
        winrt::Windows::UI::Xaml::Controls::Button m_SendButton;
    };
}