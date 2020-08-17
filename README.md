![Screenshot](https://github.com/kareldonk/QuantumGate-ChatApp-Sample/blob/master/Screenshots/broadcast_screen.jpg)

## About

This is a sample chat application built on top of the [QuantumGate](https://github.com/kareldonk/QuantumGate) peer-to-peer networking library. It's kept very basic and simple and demonstrates how to build an application and custom extender for QuantumGate. Most of the code involves working with and updating the UI of the application while QuantumGate does most the heavy lifting under the hood in order to provide secure (encrypted) and private communications.

The application listens on port `999` for incoming connections. Once peers are connected you can send messages to all of them at once via the broadcast tab, or send messages to a specific peer by opening a private tab. You can connect to other peers using their IP address via the "New Connection" tab. In case you are connecting over the Internet, you will have to use the public IP address of the peer. In addition, the peer will have to be configured to allow incoming connections on port `999` (router port forwarding and firewall configuration in Windows).

![Screenshot](https://github.com/kareldonk/QuantumGate-ChatApp-Sample/blob/master/Screenshots/private_screen.jpg)

Some of the more advanced features offered by QuantumGate have intentionally not been integrated and used to keep things simple. It's very easy to use this sample as a foundation for building a more advanced chat application, offering cover traffic, relay functionality and authentication. For more information about those features check out the [overview](https://github.com/kareldonk/QuantumGate/wiki/QuantumGate-Overview) in the QuantumGate documentation.

## Tutorial

A tutorial with much more detailed explanations for this sample application can be found in [the QuantumGate wiki](https://github.com/kareldonk/QuantumGate/wiki/Building-a-Chat-Application-using-the-QuantumGate-P2P-Networking-library) on GitHub.

## Platforms

This application is developed in C++20 and currently only supports the Microsoft Windows 10 (10.0.18362 or later) (x86/x64) platform.

## Pre-built Binaries and Releases

There are releases available on the [Releases page](https://github.com/kareldonk/QuantumGate-ChatApp-Sample/releases), some including pre-built binaries, that you can download to quickly try out. If you'd like to build from source then you can check the Manual Build section below.

## Manual Build

The `master` branch is generally kept as stable as possible so you can download the source code from there instead of from the [Releases page](https://github.com/kareldonk/QuantumGate-ChatApp-Sample/releases) if you prefer to work with the latest version.

You'll require the latest version of Microsoft Visual Studio 2019, as well as the dependencies listed below. When the paths to the dependency includes and libraries have been configured properly, building is as simple as opening the `ChatApp.sln` file in the project root with Visual Studio and issuing the build command for the entire solution.

### Dependencies

This project requires the [QuantumGate](https://github.com/kareldonk/QuantumGate) peer-to-peer networking library. The project is configured to expect the QuantumGate library files in a specific `ChatApp\QuantumGateDist` subfolder; see the separate [`README.md`](https://github.com/kareldonk/QuantumGate-ChatApp-Sample/blob/master/ChatApp/QuantumGateDist/README.md) file in that subfolder for instructions.

In addition the [C++/WinRT](https://marketplace.visualstudio.com/items?itemName=CppWinRTTeam.cppwinrt101804264) extension for Visual Studio needs to be installed as well as the [Microsoft.Windows.CppWinRT](https://www.nuget.org/packages/Microsoft.Windows.CppWinRT/) NuGet package. 

## License

The license for the QuantumGate source code can be found in the [`LICENSE`](https://github.com/kareldonk/QuantumGate-ChatApp-Sample/blob/master/LICENSE) file in the project root.
