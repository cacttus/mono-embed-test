#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <cstring>

#define Stz std::string("") +
#define BR2_USE_MONO
#define BR2_OS_LINUX

#if defined(BR2_USE_MONO)
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/mono-config.h>
#endif

#if defined(BR2_OS_LINUX)
//This gets the OS name
#include <sys/utsname.h>
//Sort of similar to windows GetLastError
#include <sys/errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#endif

typedef std::string string_t;

float randomFloat() {
  static bool _frand_init = false;
  if (!_frand_init) {
    srand((unsigned int)18093050563);
    _frand_init = true;
  }
  return float(float(rand()) / float(RAND_MAX));
}

class vec3 {
public:
  float x, y, z;
  vec3() { x = y = z = 0; }
  vec3(float dx, float dy, float dz) {
    x = dx;
    y = dy;
    z = dz;
  }
  static vec3 random() {
    return vec3(randomFloat(), randomFloat(), randomFloat());
  }
  vec3& operator=(const vec3& v) {
    this->x = v.x;
    this->y = v.y;
    this->z = v.z;
    return *this;
  }
  vec3 operator+(const vec3& v) const {
    vec3 tmp = *this;
    tmp += v;
    return tmp;
  }
  vec3& operator+=(const vec3& v) {
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
  }
  vec3 operator-(const vec3& v) const {
    vec3 tmp = *this;
    tmp -= v;
    return tmp;
  }
  vec3& operator-=(const vec3& v) {
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
  }
  vec3 operator/(const float& v) const {
    vec3 tmp = *this;
    tmp /= v;
    return tmp;
  }
  vec3& operator/=(const float& v) {
    x /= v;
    y /= v;
    z /= v;
    return *this;
  }
};

// Returns time equivalent to a number of frames at 60fps
#define FPS60_IN_MS(frames) ((int)((1.0f / 60.0f * frames) * 1000.0f))

void doHardWork_Frame(float f) {
  int ms = FPS60_IN_MS(f);
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void errorExit(const std::string& str) {
  std::cout << "Error: " << str << std::endl;
  std::cout << "Press any key to exit.. " << std::endl;
  std::cin.get();
  exit(1);
}

class Gu {
public:
  static void debugBreak() {
#if defined(BR2_OS_WINDOWS)
    DebugBreak();
#elif defined(BR2_OS_LINUX)
    raise(SIGTRAP);
#else
    OS_NOT_SUPPORTED_ERROR
#endif
  }
};

string_t executeReadOutput(const string_t& cmd) {
  string_t data = "";
#if defined(BR2_OS_LINUX)
  //This works only if VSCode launches the proper terminal (some voodoo back there);
  const int MAX_BUFFER = 8192;
  char buffer[MAX_BUFFER];
  std::memset(buffer, 0, MAX_BUFFER);
  string_t cmd_mod = Stz cmd + " 2>&1";  //redirect stderr to stdout

  FILE* stream = popen(cmd_mod.c_str(), "r");
  if (stream) {
    while (fgets(buffer, MAX_BUFFER, stream) != NULL) {
      data.append(buffer);
    }
    if (ferror(stream)) {
      std::cout << "Error executeReadOutput() " << std::endl;
    }
    clearerr(stream);
    pclose(stream);
  }
#else
  BRLogWarn("Tried to call invalid method on non-linux platform.");
  //Do nothing
#endif
  return data;
}

static MonoString* do_Complex_Engine_Work() {
  return mono_string_new(mono_domain_get(), "Hello from C++!");
}

int main(int argc, char** argv) {
  std::string code = std::string("\n") +
                     "using System;\n" +
                     "\n" +
                     "public class HelloWorld\n" +
                     "{\n" +
                     "    public static void Main(string[] args)\n" +
                     "    {\n" +
                     "        Console.WriteLine (\" Hello Mono World \");\n" +
                     "    }\n" +
                     "}\n";

  //Le'ts try to do this with popen.
  //# `pkg-config --cflags --libs mono-2`
  //

  //data/scripts/hello.cs

  char* d = getcwd(NULL, 0);
  std::cout << "CWD:" << d << std::endl;

  string_t out;
  std::cout << "Compiling hello" << std::endl;
  out = executeReadOutput("csc ./data/scripts/hello.cs -t:exe -out:./bin/hello.exe");
  std::cout << "Compile Output: " << out << std::endl;
  std::cout << "Compiling MyGame" << std::endl;
  out = executeReadOutput("csc ./data/scripts/MyGame.cs -t:library -out:./bin/MyGame.dll");
  std::cout << "Compile Output: " << out << std::endl;
  //library / module?

  //system("csc hello.cs -t:exe -out:./bin/hello.exe");
  //Gu::debugBreak();

  //So apparently this needs to be called for mono to locate assemblies.
  //  mono_set_dirs("/usr/lib","/etc"); <<- This is if you need to relocate mono to a custom directory ex. in Windows

  //Mono does not like when you call jit_init more than once. Just once when you start your app.
  mono_config_parse(NULL);
  std::string domain_name = "Script_Domain";
  MonoDomain* domain = mono_jit_init(domain_name.c_str());
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    //Test #1 - Run an .exe print output to console
    {
      MonoAssembly* assembly = assembly = mono_domain_assembly_open(domain, "./bin/hello.exe");
      if (!assembly) {
        errorExit("Assembly not found.");
      }

      int retval = mono_jit_exec(domain, assembly, argc - 1, argv + 1);
      //mono_domain_finalize(domain, 1000);
      //mono_domain_free(domain, true);
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////
    //Test #2 - Run methods from a .dll
    {
      //So to load an image the assembly also needs to be loaded.
      MonoAssembly* assembly = assembly = mono_domain_assembly_open(domain, "./bin/MyGame.dll");
      if (!assembly) {
        errorExit("Assembly not found.");
      }
      //Assemblies contain images. I'm not sure why you pass the filename if the assembly needs to be loaded anyway..?
      MonoImageOpenStatus status;
      MonoImage* mygame_image = mono_image_open("./bin/MyGame.dll", &status);
      if (mygame_image == nullptr) {
        errorExit("MyGame dll not found.");
      }

      MonoClass* className = mono_class_from_name(mygame_image, "MyGame", "MyClass");
      //[namespace.]classname:methodname[(args...)]
      MonoMethodDesc* desc = mono_method_desc_new("MyGame.MyClass:OnStart()", true);
      MonoMethod* onstart_method = mono_method_desc_search_in_class(desc, className);
      mono_method_desc_free(desc);
      if (onstart_method == nullptr) {
        errorExit("MyGame dll -> cound not locate OnStart().");
      }

      std::cout << "method found: " << mono_method_full_name(onstart_method, true) << std::endl;

      // Otherwise, invoke the constructor
      MonoObject* exception = nullptr;
      //gpointer args [4]; //These are method args gpointer / guint32 etc
      //Param 2 is the (this) pointer - NULL for static mehtods.
      MonoObject* result = mono_runtime_invoke(onstart_method, NULL, NULL, NULL);
      if (exception != nullptr) {
        errorExit(Stz "MyGame dll -> exception thrown: ??");  //How to get exception information from a MonoObject ?
      }

      mono_image_close(mygame_image);
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////
    //Test #3 - Call a C++ method from the same .dll
    {
     // mono_add_internal_call("Hello::Do_Complex_Engine_Work", do_Complex_Engine_Work);
      //TODO:
    }
  }
  mono_jit_cleanup(domain);

  ///
  std::cout << "Done. Press any key to exit.." << std::endl;
  std::cin.get();

  return 0;
}