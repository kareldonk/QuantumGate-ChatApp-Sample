#pragma once

#include "MainPage.g.h"
#include "ChatTab.h"
#include "ChatExtender.h"

namespace winrt::ChatApp::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        using ChatTabs = std::unordered_map<QuantumGate::PeerLUID, std::unique_ptr<ChatTab>>;

        MainPage();

        int32_t MyProperty();
        void MyProperty(int32_t value);

        void ShowMessage(const winrt::hstring& title, const winrt::hstring& msg) noexcept;
        void ShowErrorMessage(const winrt::hstring& msg) noexcept;
        void EnableAllChildControls(const winrt::Windows::UI::Xaml::Controls::StackPanel& element, const bool enable) noexcept;
        
        bool InitializeChatExtender() noexcept;
        void UpdateExtenderNickname();

        bool StartLocalInstance() noexcept;
        void ShutdownLocalInstance() noexcept;
        void ConnectToPeer() noexcept;
        
        ChatTab* AddChatTab(const QuantumGate::PeerLUID pluid, const std::wstring title);
        void CloseChatTab(const QuantumGate::PeerLUID pluid);

        // Event handlers
        void ConnectButtonClicked(const winrt::Windows::Foundation::IInspectable& sender,
                                  const winrt::Windows::UI::Xaml::RoutedEventArgs& e);
        void ShowConsoleButtonClicked(const winrt::Windows::Foundation::IInspectable& sender,
                                      const winrt::Windows::UI::Xaml::RoutedEventArgs& e);
        void DisconnectButtonClicked(const winrt::Windows::Foundation::IInspectable& sender,
                                     const winrt::Windows::UI::Xaml::RoutedEventArgs& e);
        void ConnectionListSelectionChanged(const winrt::Windows::Foundation::IInspectable& sender,
                                            const winrt::Windows::UI::Xaml::RoutedEventArgs& e);
        void NickNameTextBoxLostFocus(const winrt::Windows::Foundation::IInspectable& sender,
                                      const winrt::Windows::UI::Xaml::RoutedEventArgs& e);
        void NickNameTextBoxKeyUp(const winrt::Windows::Foundation::IInspectable& sender,
                                  const winrt::Windows::UI::Xaml::Input::KeyRoutedEventArgs& e);
        void PrivateChatButtonClicked(const winrt::Windows::Foundation::IInspectable& sender,
                                      const winrt::Windows::UI::Xaml::RoutedEventArgs& e);
        void OnlineButtonClicked(const winrt::Windows::Foundation::IInspectable& sender,
                                 const winrt::Windows::UI::Xaml::RoutedEventArgs& e);

        // Data members
        ChatTabs m_ChatTabs;
        mutable std::shared_mutex m_ChatTabsMutex;

        ChatTab* m_BroadCastChatTab{ nullptr };
        QuantumGate::Local m_Local;
        std::shared_ptr<ChatExtender> m_Extender;
    };
}

namespace winrt::ChatApp::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
