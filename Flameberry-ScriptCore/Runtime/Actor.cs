using System;
using System.Numerics;

namespace Flameberry.Runtime
{
	public class Actor
	{
        protected Actor() { ID = 0; }

        internal Actor(ulong id)
        {
            ID = id;
            Console.WriteLine($"INFO: Creating Actor with EntityID: {ID}");
        }

        public readonly ulong ID;

        internal delegate Vector3 Entity_GetTransform(ulong entityID);

        public virtual void OnCreate() { }
        public virtual void OnUpdate(float delta) { }
    }
}

