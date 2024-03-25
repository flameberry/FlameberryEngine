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
    }
}
