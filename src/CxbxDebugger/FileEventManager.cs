// Written by x1nixmzeng for the Cxbx-Reloaded project
//

using System;

namespace CxbxDebugger
{
    class FileEvent
    {
        public enum Type
        {
            Opened,
            Read,
            Write,
            Closed,
            FailedOpen,
        }

        public static string TypeString(Type Type)
        {
            switch (Type)
            {
                case Type.Opened: return "File Opened";
                case Type.Read: return "File Read";
                case Type.Write: return "File Write";
                case Type.Closed: return "File Closed";
                case Type.FailedOpen: return "File Open Failed";
            }

            throw new MissingMemberException();
        }

        public struct Data
        {
            public Type Type;
            public string Name;
            public uint Length;
            public uint Offset;

            public Data(Type Type, string Name, uint Length = 0, uint Offset = uint.MaxValue)
            {
                this.Type = Type;
                this.Name = Name;
                this.Length = Length;
                this.Offset = Offset;
            }
        }

        public static Data Opened(string Name)
        {
            return new Data(Type.Opened, Name);
        }

        public static Data OpenFailed(string Name)
        {
            return new Data(Type.FailedOpen, Name);
        }

        public static Data Read(string Name, uint Length, uint Offset)
        {
            return new Data(Type.Read, Name, Length, Offset);
        }

        public static Data Write(string Name, uint Length, uint Offset)
        {
            return new Data(Type.Write, Name, Length, Offset);
        }

        public static Data Closed(string Name)
        {
            return new Data(Type.Closed, Name);
        }
    }
}
