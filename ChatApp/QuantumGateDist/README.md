## QuantumGate Library Distribution

Add the QuantumGate library distribution files in this folder. If the files are missing, builds in Visual Studio will fail. You can get the distribution files by downloading a release that includes pre-built binaries from the [QuantumGate releases page](https://github.com/kareldonk/QuantumGate/releases), or, you can build them from source yourself by downloading the QuantumGate source code.

The Visual Studio build script will expect the library files (`*.dll` and `*.lib`) to be placed in subfolders according to the platform and configuration you are building for. For example, if you are building a `Release` version for the `x64` platform, the library files will need to be in the `x64\Release` subfolder. If you download pre-built binaries from the QuantumGate releases page, the files will already be arranged as required and you will just need to copy the subfolders into this one.

Alternatively you can modify the `ChatApp.vcxproj` project file and change the location of the files. Look for the following section in the project file:

```
<!-- Begin QuantumGate Dist -->
<ItemGroup>
    <Content Include="QuantumgateDist\$(Platform)\$(Configuration)\*.dll" Link="%(Filename)%(Extension)">
        <CopyToOutputDirectory>Always</CopyToOutputDirectory>
    </Content>
</ItemGroup>
<!-- End QuantumGate Dist -->
```

**Note that te script will copy all dynamic link library files (`*.dll`) from the distribution subfolder.** Make sure there are no unneccessary files present. Only the following library files are required (full names depending on the configuration):

- QuantumGate*.dll
- libcrypto*.dll
- libzstd*.dll
- zlib*.dll
