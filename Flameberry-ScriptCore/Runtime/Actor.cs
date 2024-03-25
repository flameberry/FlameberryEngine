using Flameberry.Managed;

namespace Flameberry
{
    public class Actor
    {
        protected Actor() { ID = 0; }

        internal Actor(ulong id)
        {
            ID = id;
        }

        public readonly ulong ID;

        public bool HasComponent<T>() where T : Component, new()
        {
            //bool hasComponent = InternalCalls.Entity_HasComponent(ID, typeof(T).GetHashCode());
            //return hasComponent;
            return true;
        }

        public T? GetComponent<T>() where T : Component, new()
        {
            if (!HasComponent<T>())
                return null;
            return new T() { Actor = this };
        }
    }
}
