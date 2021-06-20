
//csc hello.cs -t:exe -out:./bin/hello.exe

using System;
using System.Runtime.CompilerServices;

class vec3 
{ 
  public float x, y, z;
  public vec3(){}
  public vec3(float dx, float dy, float dz){x=dx;y=dy;z=dz;}
  public float length(){ return (float)Math.Sqrt(x*x + y*y + z*z); }
}

namespace MyGame 
{
    public class MyClass
    {
        //This will bind to the method from C++
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        extern static string Do_Complex_Engine_Work();

        public static void OnStart()
        {
            Console.WriteLine("OnStart called");
        }
        public static void OnUpdate()
        {
            //Do_Complex_Engine_Work();
            Console.WriteLine("OnUpdate called");
        }
        public static void OnExit()
        {
            Console.WriteLine("OnExit called");
        }
    }
}
