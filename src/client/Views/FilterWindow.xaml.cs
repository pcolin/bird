using client.ViewModels;
using Prism.Commands;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace client.Views
{
    /// <summary>
    /// Interaction logic for FilterWindow.xaml
    /// </summary>
    public partial class FilterWindow : Window
    {
        public FilterWindow()
        {
            InitializeComponent();
            var vm = new FilterWindowViewModel();
            this.DataContext = vm;
            this.ListBoxExchanges.ItemsSource = vm.ExchangesView;
        }

        private void DropDownButton_Click(object sender, RoutedEventArgs e)
        {
            //this.ListBoxExchanges.ItemsSource = (this.DataContext as FilterWindowViewModel).SelectedExchanges;
        }

    }

    public class FilterWindowViewModel : BindableBase
    {
        public FilterWindowViewModel()
        {
            var items = new ObservableCollection<FilterItem>();
            items.Add(new FilterItem() { Exchange = Proto.Exchange.Dce, Option = "m1901-C-2500", Type = Proto.OptionType.Call });
            items.Add(new FilterItem() { Exchange = Proto.Exchange.Dce, Option = "m1901-P-2500", Type = Proto.OptionType.Put });
            items.Add(new FilterItem() { Exchange = Proto.Exchange.Czce, Option = "SR901C2500", Type = Proto.OptionType.Call });
            items.Add(new FilterItem() { Exchange = Proto.Exchange.Czce, Option = "SR901P3500", Type = Proto.OptionType.Put });
            this.Items = items;

            Func<Proto.Exchange, string> exchangeFunc = e => e.ToString();
            var exchanges = new ObservableCollection<FilterItem<Proto.Exchange>>();
            //Action<bool, Proto.Exchange> action = (selected, exchange) => FilterExchange(selected, exchange);
            exchanges.Add(new FilterItem<Proto.Exchange>(this.FilterExchange));
            exchanges.Add(new FilterItem<Proto.Exchange>(this.FilterExchange, exchangeFunc, Proto.Exchange.Dce));
            exchanges.Add(new FilterItem<Proto.Exchange>(this.FilterExchange, exchangeFunc, Proto.Exchange.Czce));
            this.SelectedExchanges = exchanges;

            this.CallCommand = new DelegateCommand(this.CallExecute);
            this.ExchangeCommand = new DelegateCommand(this.ExchangeExecute);

            this.FilterExchangeCommand = new DelegateCommand(this.FilterExchangeExecute);
        }

        public ICollectionView ItemViews { get; set; }
        
        private ObservableCollection<FilterItem> items;
        public ObservableCollection<FilterItem> Items
        {
            get { return items; }
            set
            {
                if (SetProperty(ref items, value))
                {
                    ItemViews = CollectionViewSource.GetDefaultView(value);
                    ItemViews.Filter = this.Filter;
                }
            }
        }

        public ICollectionView ExchangesView { get; set; }
        private ObservableCollection<FilterItem<Proto.Exchange>> selectedExchanges;
        public ObservableCollection<FilterItem<Proto.Exchange>> SelectedExchanges
        {
            get { return selectedExchanges; }
            set
            {
                if (SetProperty(ref selectedExchanges, value))
                {
                    ExchangesView = CollectionViewSource.GetDefaultView(value);
                    //ItemViews.Filter = this.Filter;
                }
            }
        }

        private bool isChecked;
        public bool IsChecked
        {
            get { return isChecked; }
            set
            {
                if (SetProperty(ref isChecked, value))
                {
                    for (int i = 1; i < selectedExchanges.Count; ++i)
                    {
                        selectedExchanges[i].IsSelected = value;
                    }
                        //foreach (var item in selectedExchanges)
                        //{
                        //    item.IsSelected = value;
                        //}
                }
            }
        }

        public DelegateCommand FilterExchangeCommand { get; set; }

        public DelegateCommand CallCommand { get; set; }
        public DelegateCommand ExchangeCommand { get; set; }

        HashSet<Proto.Exchange> excludedExchanges = new HashSet<Proto.Exchange>();
        HashSet<Proto.OptionType> excludedOptionTypes = new HashSet<Proto.OptionType>();
        HashSet<string> excludedOptions = new HashSet<string>();

        public void FilterExchange(bool selected, bool all, Proto.Exchange exchange)
        {
            if (selected)
            {
                if (all)
                {
                    this.excludedExchanges.Clear();
                    for (int i = 1; i < this.selectedExchanges.Count; ++i)
                    {
                        this.selectedExchanges[i].SetIsSelected(true);
                        //this.SelectedExchanges[i].IsSelected = true;
                    }
                    IsChecked = true;
                }
                else
                {
                    //this.selectedExchanges[0].SetIsSelected(true);
                    this.excludedExchanges.Remove(exchange);
                }
            }
            else if (all)
            {
                for (int i = 1; i < this.selectedExchanges.Count; ++i)
                {
                    this.selectedExchanges[i].SetIsSelected(false);
                    this.excludedExchanges.Add(this.selectedExchanges[i].Item);
                }
                IsChecked = false;
            }
            else
            {
                this.selectedExchanges[0].SetIsSelected(false);
                this.excludedExchanges.Add(exchange);
            }
            //this.ItemViews = CollectionViewSource.GetDefaultView(this.selectedExchanges);
            this.ItemViews.Refresh();
            this.ExchangesView.Refresh();
        }

        void FilterOption(bool selected, bool all, string option)
        {
            if (selected)
            {
                this.excludedOptions.Remove(option);
            }
            else
            {
                this.excludedOptions.Add(option);
            }
            this.ItemViews.Refresh();
        }

        void CallExecute()
        {
            //if (this.ItemViews.Filter == null)
            //{
            //    this.ItemViews.Filter += CallFilter;
            //}
            //else
            //{
            //    this.ItemViews.Filter -= CallFilter;
            //}
        }

        void ExchangeExecute()
        {
            //if (this.ItemViews.Filter == null)
            //{
            //    this.ItemViews.Filter += ExchangeFilter;
            //}
            //else
            //{
            //    this.ItemViews.Filter -= ExchangeFilter;
            //}
        }

        void FilterExchangeExecute()
        {
            var exchanges = (from it in this.ItemViews.Cast<FilterItem>() orderby it.Exchange select it.Exchange).Distinct();
            selectedExchanges.Clear();
            foreach (var exch in exchanges)
            {
                //select
            }
        }

        bool Filter(object item)
        {
            var it = item as FilterItem;
            return !(excludedExchanges.Contains(it.Exchange) || excludedOptions.Contains(it.Option) || excludedOptionTypes.Contains(it.Type));
        }

        bool CallFilter(object item)
        {
            FilterItem it = item as FilterItem;
            return it.Type == Proto.OptionType.Call;
        }

        public bool ExchangeFilter(object item)
        {
            FilterItem it = item as FilterItem;
            return it.Exchange == Proto.Exchange.Dce;
        }
    }

    public class SelectedItem<T> : BindableBase
    {
        public SelectedItem(bool selected, T item, Action<bool, T> action)
        {
            this.action = action;
            this.IsSelected = selected;
            this.Item = item;
        }

        private bool isSelected;
        public bool IsSelected
        {
            get { return isSelected; }
            set
            {
                if (SetProperty(ref isSelected, value))
                {
                    //if (value)
                    //{
                    //    this.ExcludedItems.Add(Item);
                    //}
                    //else
                    //{
                    //    this.ExcludedItems.Remove(Item);
                    //}
                    action(value, Item);
                }
            }
        }

        public T Item { get; private set; }

        Action<bool, T> action;
        //public HashSet<T> ExcludedItems { get; set; }
    }

    public class FilterItem : BindableBase
    {
        private Proto.Exchange exchange;
        public Proto.Exchange Exchange
        {
            get { return exchange; }
            set { SetProperty(ref exchange, value); }
        }

        private string option;
        public string Option
        {
            get { return option; }
            set { SetProperty(ref option, value); }
        }

        private Proto.OptionType type;
        public Proto.OptionType Type
        {
            get { return type; }
            set { SetProperty(ref type, value); }
        }        
    }
}
