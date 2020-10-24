// Written by x1nixmzeng for the Cxbx-Reloaded project
//

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CxbxDebugger
{
    enum FileEventType
    {
        Opened,
        Read,
        Write,
        Closed,
    }

    struct FileEvents
    {
        public FileEventType Type;
        public string Name;
        public uint Length;
        public uint Offset;
        public bool Succeeded;

        public FileEvents(FileEventType Type, string Name, bool Succeeded, uint Length = 0, uint Offset = uint.MaxValue)
        {
            this.Type = Type;
            this.Name = Name;
            this.Succeeded = Succeeded;
            this.Length = Length;
            this.Offset = Offset;
        }

        public static FileEvents Opened(string Name, bool Succeeded)
        {
            return new FileEvents(FileEventType.Opened, Name, Succeeded);
        }

        public static FileEvents Read(string Name, uint Length, uint Offset)
        {
            return new FileEvents(FileEventType.Read, Name, true, Length, Offset);
        }

        public static FileEvents Write(string Name, uint Length, uint Offset)
        {
            return new FileEvents(FileEventType.Write, Name, true, Length, Offset);
        }

        public static FileEvents Closed(string Name)
        {
            return new FileEvents(FileEventType.Closed, Name, true);
        }
    }
}
