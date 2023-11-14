using System;
using System.Numerics;
using Flameberry.Managed;

namespace Flameberry.Runtime
{
    public class Actor
    {
        protected Actor() { ID = 0; }

        internal Actor(ulong id)
        {
            ID = id;
        }

        internal ulong ID; // TODO: Figure a way to make this readonly

        public virtual void OnCreate() { }
        public virtual void OnUpdate(float delta) { }

        //public bool HasComponent<T>() where T : Component, new()
        //{
        //    unsafe
        //    {
        //        bool hasComponent = InternalCallStorage.Entity_HasComponent(ID, typeof(T).GetHashCode());
        //        return hasComponent;
        //    }
        //}

        //public T? GetComponent<T>() where T: Component, new ()
        //{
        //    if (!HasComponent<T>())
        //        return null;
        //    return new T() { Actor = this };
        //}
    }
}

