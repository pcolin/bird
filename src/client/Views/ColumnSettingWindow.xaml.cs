using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
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
    /// Interaction logic for ColumnSettingWindow.xaml
    /// </summary>
    public partial class ColumnSettingWindow : Window
    {
        public ColumnSettingWindow(DataGrid dg, Dictionary<int, string> formats)
        {
            InitializeComponent();

            this.dg = dg;
            this.formats = formats;

            List<SettingItem> settings = new List<SettingItem>();
            for (int i = 0; i < dg.Columns.Count; ++i)
            {
                var column = dg.Columns[i];
                int decimalPlace = -1;
                string format = null;
                if (formats.TryGetValue(column.DisplayIndex, out format))
                {
                    decimalPlace = Convert.ToInt32(format.Substring(1));
                }

                settings.Add(new SettingItem()
                {
                    Index = i,
                    Header = column.Header.ToString(),
                    DisplayIndex = column.DisplayIndex,
                    Visible = column.Visibility == System.Windows.Visibility.Visible,
                    DecimalPlace = decimalPlace,
                    IsDecimal = decimalPlace != -1
                });
            }
            settings.Sort(Comparer<SettingItem>.Create((x, y) => x.DisplayIndex.CompareTo(y.DisplayIndex)));
            items = new ObservableCollection<SettingItem>(settings);
            this.SettingDataGrid.ItemsSource = items;
        }

        DataGrid dg;
        Dictionary<int, string> formats;
        ObservableCollection<SettingItem> items;

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            //MessageBox.Show("settingwindow in " + System.Threading.Thread.CurrentThread.ManagedThreadId);
        }

        private void ButtonUP_Click(object sender, RoutedEventArgs e)
        {
            int index = this.SettingDataGrid.SelectedIndex;
            var tmp = this.items[index];
            this.items[index] = this.items[index - 1];
            this.items[index - 1] = tmp;
            this.SettingDataGrid.SelectedIndex = index - 1;
        }

        private void ButtonDown_Click(object sender, RoutedEventArgs e)
        {
            int index = this.SettingDataGrid.SelectedIndex;
            var tmp = this.items[index];
            this.items[index] = this.items[index + 1];
            this.items[index + 1] = tmp;
            this.SettingDataGrid.SelectedIndex = index + 1;
        }

        private void SettingDataGrid_SelectedCellsChanged(object sender, SelectedCellsChangedEventArgs e)
        {
            int index = this.SettingDataGrid.SelectedIndex;
            this.ButtonUP.IsEnabled = index > this.dg.FrozenColumnCount;
            this.ButtonDown.IsEnabled = index >= this.dg.FrozenColumnCount && index < this.SettingDataGrid.Items.Count - 1;
        }

        private void SettingDataGrid_LoadingRow(object sender, DataGridRowEventArgs e)
        {
            e.Row.Header = e.Row.GetIndex().ToString();
        }

        private void ButtonOK_Click(object sender, RoutedEventArgs e)
        {
            formats.Clear();
            for (int i = 0; i < items.Count(); ++i)
            {
                var column = this.dg.Columns[items[i].Index];
                column.DisplayIndex = i;
                column.Visibility = items[i].Visible ? Visibility.Visible : Visibility.Hidden;
                if (items[i].DecimalPlace >= 0)
                {
                    formats[i] = "F" + items[i].DecimalPlace;
                }
            }
            this.Close();
        }

        private void ButtonCancel_Click(object sender, RoutedEventArgs e)
        {
            this.Close();
        }
    }

    public class SettingItem
    {
        public int Index { get; set; }
        public int DisplayIndex { get; set; }
        public string Header { get; set; }
        public int DecimalPlace { get; set; }
        public bool IsDecimal { get; set; }
        public bool Visible { get; set; }
    }

    public class SelectedIndexToBooleanConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            int index = (int)value;
            return index > 0;
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
