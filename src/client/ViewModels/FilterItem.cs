using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.ViewModels
{
    public class FilterItem<T> : BindableBase
    {
        public FilterItem(Action<bool, bool, T> action)
        {
            this.action = action;
            this.all = true;
        }

        public FilterItem(Action<bool, bool, T> action, Func<T, string> func, T item)
        {
            this.action = action;
            this.func = func;
            this.all = false;
            this.Item = item;
        }

        //public FilterItem(Action<bool, bool, T> action, Func<T, string> func, bool selected, bool all, T item)
        //{
        //    this.action = action;
        //    this.isSelected = selected;
        //    this.all = all;
        //    this.Item = item;
        //}

        private bool isSelected = true;
        public bool IsSelected
        {
            get { return isSelected; }
            set
            {
                if (SetProperty(ref isSelected, value))
                {
                    action(value, all, Item);
                }
            }
        }

        public void SetIsSelected(bool selected)
        {
            SetProperty(ref isSelected, selected);
        }

        public T Item { get; private set; }

        public string Display { get { return all ? "(Select All)" : func(Item); } }
        
        Action<bool, bool, T> action;
        Func<T, string> func;
        bool all;
    }
}
