using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeWrapper
{
    [Core.Attributes.RuntimeWrapper("nox::Transform")]
    public class Transform : Core.RuntimeObject
    {
        public Nox.Position LocalPosition { get; set; }
        public System.Numerics.Vector3 LocalScale { get; set; }

    }
}
