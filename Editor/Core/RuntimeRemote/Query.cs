using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeRemote
{
	public abstract class Query : Core.RuntimeRemote.Entity
	{
		public virtual Core.RuntimeRemote.Response? Execute() => null;
	}
}
