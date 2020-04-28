# blockchain-iot-security


## Dependencies

### nc (netcat) - BSD Version
You must have the BSD version of netcat installed. Other versions of netcat do not provide the -U flag for Unix Domain Sockets. This is used from EthInterface::eth_ipc_request().

#### Debian, Ubuntu, Raspbian:
sudo apt update && sudo apt install netcat-openbsd


## ethabi
Compile and install the ethabi binary. It must use only this one specific commit from the project as listed. This is because a bug was fixed on one commit ([https://github.com/openethereum/ethabi/issues/162]), then the commit following that one changed the CLI fairly radically and broke compatibility.

First install rustc & cargo as directed at [https://www.rust-lang.org/tools/install]

Ensure that the necessary commands to `~/.profile` as specified, then:

`cargo install --git https://github.com/paritytech/ethabi.git --rev 7de908fccb2426950dc38a412c35bf1c5b1f6561`

Test ethabi works: `ethabi -h`


### Boost Library
Version 1.67 or greater.

#### Debian, Ubuntu, Raspbian:

`sudo apt update && sudo apt install libboost-all-dev`

### Go Ethereum (Geth) Client

```
git clone https://github.com/ethereum/go-ethereum.git
cd go-ethereum
git checkout v1.9.9
make
```

### Solidity Compiler (solc)

#### Ubuntu:
`sudo apt update && sudo apt install solc`


#### Debian, Raspbian:
The Ubuntu based apt repository for Solidity is not compatible with Debian. Use the Snap package manager to install instead. Version 0.6.0 or greater is required.

`sudo apt install -y snapd && sudo snap install solc`

If the above command does not provide 0.6.0 or greater, then:
```
sudo snap remove solc
sudo snap install solc --edge
```

#### MacOS:
Install the homebrew package manager, then run:

```
brew tap ethereum/ethereum
brew install solidity
```

### Others
Some of the git submodules may need to be compiled and installed. Please do so as required.



## C/C++ code is formatted with clang-format in VSCode using the setting:
_**C_Cpp:Clang_format_fallback**_
```
{ BasedOnStyle: WebKit, UseTab: Always, IndentWidth: 4, BreakBeforeBraces: Allman, AllowShortIfStatementsOnASingleLine: false, IndentCaseLabels: true, ColumnLimit: 0, TabWidth: 4, AlwaysBreakAfterDefinitionReturnType: All, MaxEmptyLinesToKeep: 3, NamespaceIndentation: None, AccessModifierOffset: 0 }
```