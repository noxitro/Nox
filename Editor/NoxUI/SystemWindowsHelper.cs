using System;
using System.Collections.Generic;
using System.Text;

namespace NoxUI
{
	public interface IDependencyObject<Owner> where Owner : System.Windows.DependencyObject
	{
		public delegate void Callback(Owner owner, in System.Windows.DependencyPropertyChangedEventArgs e);

		protected static System.Windows.DependencyProperty Register<Property>(string name, Property defaultValue, Callback propertyChangedCallback)
		{
			void Wrapper(System.Windows.DependencyObject d, System.Windows.DependencyPropertyChangedEventArgs e)
			{
				propertyChangedCallback((Owner)d, in e);
			}
			return System.Windows.DependencyProperty.Register(
				name, 
				typeof(Property),
				typeof(Owner),
				new System.Windows.PropertyMetadata(defaultValue, Wrapper));
		}

		protected static System.Windows.DependencyProperty Register<Property>(string name)
		{
			return System.Windows.DependencyProperty.Register(name, typeof(Property), typeof(Owner));
		}
		protected static System.Windows.DependencyProperty Register<Property>(string name, System.Windows.PropertyMetadata typeMetadata)
		{
			return System.Windows.DependencyProperty.Register(name, typeof(Property), typeof(Owner), typeMetadata);
		}

		protected static System.Windows.DependencyProperty Register<Property>(string name, System.Windows.PropertyMetadata typeMetadata, System.Windows.ValidateValueCallback validateValueCallback)
		{
			return System.Windows.DependencyProperty.Register(name, typeof(Property), typeof(Owner), typeMetadata, validateValueCallback);
		}

		protected static System.Windows.DependencyProperty Register<Property>(string name, object defaultValue, System.Windows.PropertyChangedCallback propertyChangedCallback)
		{
			return System.Windows.DependencyProperty.Register(name, typeof(Property), typeof(Owner), new System.Windows.PropertyMetadata(defaultValue, propertyChangedCallback));
		}
		protected static System.Windows.DependencyProperty Register<Property>(string name, System.Windows.PropertyChangedCallback propertyChangedCallback)
		{
			return Register<Property>(name, string.Empty, propertyChangedCallback);
		}
	}

	public static class SystemWindowsHelper
	{
		public static System.Windows.DependencyProperty Register<Property, Owner>(string name)
		{
			return System.Windows.DependencyProperty.Register(name, typeof(Property), typeof(Owner));
		}
	}
}
