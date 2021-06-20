/*
  Design a graphics system where:
    Scenes update independently of rendering.
      Perform unlimited updates.
      Render scene.
    Window families share the same scene.
      Window Family = sharing same GL context.
    Multiple Scenes supported.
    Rendering Pipeline shared across
      GSurface
      GWindow

  * No point of running update async anyway, if we can't render
  asynchronously (without real intervention).

*/

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/mono-config.h>

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
  std::cout << str << std::endl;
  std::cin.get();
  exit(1);
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

  //  mono_set_dirs("/usr/lib","/etc");

  //So apparently this needs to be called for mono to locate assemblies.
  mono_config_parse(NULL);

  std::string domain_name = "asdf-does-not-matter";
  MonoDomain* domain;
  domain = mono_jit_init(domain_name.c_str());

  MonoAssembly* assembly;
  assembly = mono_domain_assembly_open(domain, "./hello.exe");
  if (!assembly) {
    errorExit("Assembly not found.");
  }

  int retval = mono_jit_exec(domain, assembly, argc - 1, argv + 1);

  mono_jit_cleanup(domain);

  std::cout << "Done. Press any key to exit.." << std::endl;
  std::cin.get();

  return 0;
}