using client.ViewModels;
using Dragablz;
using Microsoft.Practices.Unity;
using Prism.Events;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using System.Xml;

namespace client.Views
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private Dictionary<string, Tuple<double, double, double, double>> placements = new Dictionary<string, Tuple<double, double, double, double>>();

        public MainWindow()
        {
            InitializeComponent();
            //this.DataContext = new MainWindowViewModel();
        }

        private void LoginMenuItem_Click(object sender, RoutedEventArgs e)
        {
            LoginWindow login = new LoginWindow();
            login.DataContext = this.DataContext;
            login.ShowDialog();
        }

        private void OptionsMenuItem1_Click(object sender, RoutedEventArgs e)
        {
            MainWindowViewModel vm = this.DataContext as MainWindowViewModel;
            OptionWindow w = vm.Container.Resolve<OptionWindow>(vm.Exchange1.ToString());
            ShowOptionWindow(w);
        }

        private void OptionsMenuItem2_Click(object sender, RoutedEventArgs e)
        {
            MainWindowViewModel vm = this.DataContext as MainWindowViewModel;
            OptionWindow w = vm.Container.Resolve<OptionWindow>(vm.Exchange2.ToString());
            ShowOptionWindow(w);
        }

        private void OptionsMenuItem3_Click(object sender, RoutedEventArgs e)
        {
            MainWindowViewModel vm = this.DataContext as MainWindowViewModel;
            OptionWindow w = vm.Container.Resolve<OptionWindow>(vm.Exchange3.ToString());
            ShowOptionWindow(w);
        }

        private void OptionsMenuItem4_Click(object sender, RoutedEventArgs e)
        {
            MainWindowViewModel vm = this.DataContext as MainWindowViewModel;
            OptionWindow w = vm.Container.Resolve<OptionWindow>(vm.Exchange4.ToString());
            ShowOptionWindow(w);
        }

        private void ShowOptionWindow(OptionWindow w)
        {
            w.Dispatcher.Invoke(() =>
            {
                if (w.IsVisible == false)
                {
                    w.Show();
                }
                w.Topmost = true;
                w.Topmost = false;
            });
        }


        private void Window_Initialized(object sender, EventArgs e)
        {
            MainWindowViewModel vm = this.DataContext as MainWindowViewModel;
            vm.Initialize();

            /// load layout
            Layout = vm.Layout;
            XmlReader reader = XmlReader.Create(System.AppDomain.CurrentDomain.BaseDirectory + "\\layout\\" + Layout + "\\WindowPlacement.xml");
            while (reader.Read())
            {
                if (reader.NodeType == XmlNodeType.Element && reader.Name == "Window")
                {
                    placements.Add(reader.GetAttribute("Name"), new Tuple<double, double, double, double>(Convert.ToDouble(reader.GetAttribute("Left")),
                        Convert.ToDouble(reader.GetAttribute("Top")), Convert.ToDouble(reader.GetAttribute("Width")), Convert.ToDouble(reader.GetAttribute("Height"))));
                }
            }
            //MessageBox.Show("mainwindow in " + Thread.CurrentThread.ManagedThreadId);
            PlaceWindow(this, placements["MainWindow"]);

            if (vm.Exchange1Visible)
            {
                NewOptionViewWindow(vm.Container, vm.Exchange1);
            }
            if (vm.Exchange2Visible)
            {
                NewOptionViewWindow(vm.Container, vm.Exchange2);
            }
            if (vm.Exchange3Visible)
            {
                NewOptionViewWindow(vm.Container, vm.Exchange3);
            }
            if (vm.Exchange4Visible)
            {
                NewOptionViewWindow(vm.Container, vm.Exchange4);
            }
        }

        //private void Window_Loaded(object sender, RoutedEventArgs e)
        //{

        //}

        private void NewOptionViewWindow(IUnityContainer container, Proto.Exchange exchange)
        {
            Thread t = new Thread(() =>
            {
                OptionWindow w = new OptionWindow(container, exchange);
                PlaceWindow(w, placements["OptionWindow_" + exchange]);
                w.Show();

                container.RegisterInstance<OptionWindow>(exchange.ToString(), w);
                w.Closed += (sender2, e2) => w.Dispatcher.InvokeShutdown();

                System.Windows.Threading.Dispatcher.Run();
            });
            t.SetApartmentState(ApartmentState.STA);
            //t.IsBackground = true;
            t.Start();
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            MainWindowViewModel vm = this.DataContext as MainWindowViewModel;
            vm.Stop();

            ///vm.Container.Resolve<EventAggregator>().GetEvent<CloseWindowEvent>().Publish();
            XmlWriterSettings settings = new XmlWriterSettings();
            settings.Indent = true;
            XmlWriter writer = XmlWriter.Create(System.AppDomain.CurrentDomain.BaseDirectory + "\\layout\\" + Layout + "\\WindowPlacement.xml", settings);
            writer.WriteStartDocument();
            writer.WriteStartElement("Windows");

            WriteWindowPlacement(writer, this, "MainWindow");

            /// Close OptionWindow
            if (vm.Exchange1Visible)
            {
                var w = vm.Container.Resolve<OptionWindow>(vm.Exchange1.ToString());
                w.SaveAndClose(writer);
            }
            if (vm.Exchange2Visible)
            {
                var w = vm.Container.Resolve<OptionWindow>(vm.Exchange2.ToString());
                w.SaveAndClose(writer);
            }
            if (vm.Exchange3Visible)
            {
                var w = vm.Container.Resolve<OptionWindow>(vm.Exchange3.ToString());
                w.SaveAndClose(writer);
            }
            if (vm.Exchange4Visible)
            {
                var w = vm.Container.Resolve<OptionWindow>(vm.Exchange4.ToString());
                w.SaveAndClose(writer);
            }
            writer.WriteEndElement();
            writer.WriteEndDocument();
            writer.Flush();
            writer.Close();
        }

        static public void WriteWindowPlacement(XmlWriter writer, Window window, string name)
        {
            writer.WriteStartElement("Window");
            writer.WriteAttributeString("Name", name);
            writer.WriteAttributeString("Left", window.Left.ToString());
            writer.WriteAttributeString("Top", window.Top.ToString());
            writer.WriteAttributeString("Width", window.ActualWidth.ToString());
            writer.WriteAttributeString("Height", window.ActualHeight.ToString());
            writer.WriteEndElement();
            writer.Flush();
        }

        static public void PlaceWindow(Window window, Tuple<double, double, double, double> placement)
        {
            window.Left = placement.Item1;
            window.Top = placement.Item2;
            window.Width = placement.Item3;
            window.Height = placement.Item4;
        }

        static public void LoadDataGridLayout(string name, DataGrid dg, Dictionary<int, string> formats)
        {
            XmlReader reader = XmlReader.Create(System.AppDomain.CurrentDomain.BaseDirectory + "\\layout\\" + MainWindow.Layout + "\\datagrid\\" + name);
            while (reader.Read())
            {
                if (reader.NodeType == XmlNodeType.Element)
                {
                    if (reader.Name == "Column")
                    {
                        int index = Convert.ToInt32(reader.GetAttribute("Index"));
                        int displayIndex = Convert.ToInt32(reader.GetAttribute("DisplayIndex"));
                        dg.Columns[index].DisplayIndex = displayIndex;
                        dg.Columns[index].Width = Convert.ToDouble(reader.GetAttribute("Width"));
                        dg.Columns[index].Visibility = Convert.ToBoolean(reader.GetAttribute("Visible")) ? Visibility.Visible : Visibility.Hidden;
                        string decimalPlace = reader.GetAttribute("Decimal");
                        if (decimalPlace != null)
                        {
                            formats[displayIndex] = decimalPlace;
                        }
                    }
                    //else if (reader.Name == "Columns")
                    //{
                    //    dg.Height = Convert.ToDouble(reader.GetAttribute("Height"));
                    //}
                }
            }
        }

        static public void SaveDataGridLayout(string name, DataGrid dg, Dictionary<int, string> formats)
        {
            XmlWriterSettings settings = new XmlWriterSettings();
            settings.Indent = true;
            XmlWriter writer = XmlWriter.Create(System.AppDomain.CurrentDomain.BaseDirectory + "\\layout\\" + Layout + "\\datagrid\\" + name, settings);
            writer.WriteStartDocument();
            writer.WriteStartElement("Columns");
            writer.WriteAttributeString("Height", dg.ActualHeight.ToString());
            int index = 0;
            foreach (var column in dg.Columns)
            {
                writer.WriteStartElement("Column");
                writer.WriteAttributeString("Index", index.ToString());
                writer.WriteAttributeString("DisplayIndex", column.DisplayIndex.ToString());
                writer.WriteAttributeString("Header", column.Header.ToString());
                writer.WriteAttributeString("Width", column.ActualWidth.ToString("F2"));
                writer.WriteAttributeString("Visible", (column.Visibility == Visibility.Visible).ToString());
                string decimalPlaces = null;
                if (formats.TryGetValue(column.DisplayIndex, out decimalPlaces))
                {
                    writer.WriteAttributeString("Decimal", decimalPlaces);
                }
                writer.WriteEndElement();
                ++index;
            }

            writer.WriteEndElement();
            writer.WriteEndDocument();
            writer.Flush();
            writer.Close();
        }

        static public string Layout = null;

        private void MenuItem_Click(object sender, RoutedEventArgs e)
        {

        }

        private void MenuItemDataGrid_Click(object sender, RoutedEventArgs e)
        {
            DataGrid dg = new DataGrid();
            dg.Columns.Add(new DataGridTextColumn() { Header = "Symbol", DisplayIndex = 0 });
            dg.Columns.Add(new DataGridTextColumn() { Header = "Bid", DisplayIndex = 2, Visibility = System.Windows.Visibility.Hidden });
            dg.Columns.Add(new DataGridTextColumn() { Header = "Ask", DisplayIndex = 1 });

            Dictionary<int, string> formats = new Dictionary<int, string>();
            formats[0] = "F2";

            ColumnSettingWindow w = new ColumnSettingWindow(dg, formats);
            w.Show();
        }
    }

    public class StatusColorConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            ConnectStatus status = (ConnectStatus)value;
            switch (status)
            {
                case ConnectStatus.Connected:
                    return "Green";
                case ConnectStatus.Disconnected:
                    return "Red";
                case ConnectStatus.Disabled:
                    return "Gray";
                default:
                    return "Gray";
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
