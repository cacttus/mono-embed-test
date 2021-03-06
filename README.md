# mono-embed-test
Test using Mono as a C# game engine scripting language.

* Test performance overhead of mono vs c++.
* Test memory usage and unloading assemblies by creating/unloading app domains.
* Test creating methods to marshall data between Mono and C++.

Bugs:
  mono_jit_exec produces an error running the application outside of the dev environment - Ubuntu.

Not tested in Windows/Mac

### Install steps - Ubuntu
All the following is copied from the mono development page.
Install the mono-devel:
https://www.mono-project.com/download/stable/#download-lin
```
    sudo apt install gnupg ca-certificates
    sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 3FA7E0328081BFF6A14DA29AA6A19B38D3D831EF
    echo "deb https://download.mono-project.com/repo/ubuntu stable-focal main" | sudo tee /etc/apt/sources.list.d/mono-official-stable.list
    sudo apt update
    sudo apt install mono-devel`
```
### Run a test
```
    using System;

    public class HelloWorld
    {
        public static void Main(string[] args)
        {
            Console.WriteLine ("Hello Mono World");
        }
    }
```
### Compile & Run
* csc hello.cs
* mono hello.exe

### Winforms Helloworld
    using System;
    using System.Windows.Forms;

    public class HelloWorld : Form
    {
        static public void Main ()
        {
            Application.Run (new HelloWorld ());
        }

        public HelloWorld ()
        {
            Text = "Hello Mono World";
        }
    }

csc hello.cs -t:exe -out:./bin/hello.exe


