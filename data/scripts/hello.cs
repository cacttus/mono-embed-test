
//csc hello.cs -t:exe -out:./bin/hello.exe

using System;

class vec3 
{ 
  public float x, y, z;
  public vec3(){}
  public vec3(float dx, float dy, float dz){x=dx;y=dy;z=dz;}
  public float length(){ return (float)Math.Sqrt(x*x + y*y + z*z); }
}

public class Hello 
{
  public static void Main(string[] args)
  {
    vec3 v = new vec3(1,1,1);
    Console.WriteLine("Hello mono world, length:" + v.length());
  }
}
