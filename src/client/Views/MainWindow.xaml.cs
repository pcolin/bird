using client.ViewModels;
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
        private Dictionary<string, Tuple<bool, double, double, double, double>> placements = new Dictionary<string, Tuple<bool, double, double, double, double>>();

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
            XmlReader reader = null;
            try
            {
                reader = XmlReader.Create(System.AppDomain.CurrentDomain.BaseDirectory + "\\layout\\" + Layout + "\\WindowPlacement.xml");
                while (reader.Read())
                {
                    if (reader.NodeType == XmlNodeType.Element && reader.Name == "Window")
                    {
                        placements.Add(reader.GetAttribute("Name"), new Tuple<bool, double, double, double, double>(Convert.ToBoolean(reader.GetAttribute("Visible")),
                            Convert.ToDouble(reader.GetAttribute("Left")), Convert.ToDouble(reader.GetAttribute("Top")), Convert.ToDouble(reader.GetAttribute("Width")),
                            Convert.ToDouble(reader.GetAttribute("Height"))));
                    }
                }
            }
            catch (Exception) { }
            finally
            {
                if (reader != null) reader.Close();
            }

            PlaceWindow(this, "MainWindow");

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

            NewExchangeWindow(vm.Container);
            NewRateWindow(vm.Container);
            NewVolatilityWindow(vm.Container);
            NewMessageWindow(vm.Container);
            NewPortfolioWindow(vm.Container);
            NewStrategyWindow(vm.Container);
        }

        private void NewOptionViewWindow(IUnityContainer container, Proto.Exchange exchange)
        {
            Thread t = new Thread(() =>
            {
                OptionWindow w = new OptionWindow(container, exchange);
                if (PlaceWindow(w, "OptionWindow_" + exchange))
                {
                    w.Show();
                }

                container.RegisterInstance<OptionWindow>(exchange.ToString(), w);
                w.Closed += (sender2, e2) => w.Dispatcher.InvokeShutdown();

                System.Windows.Threading.Dispatcher.Run();
            });
            t.SetApartmentState(ApartmentState.STA);
            //t.IsBackground = true;
            t.Start();
        }

        private void NewExchangeWindow(IUnityContainer container)
        {
            ExchangeWindow w = new ExchangeWindow();
            w.DataContext = new ExchangeWindowViewModel(container);
            if (PlaceWindow(w, "ExchangeWindow"))
            {
                w.Show();
            }
            container.RegisterInstance<ExchangeWindow>(w);
        }

        private void NewRateWindow(IUnityContainer container)
        {
            RateWindow w = new RateWindow();
            w.DataContext = new RateWindowViewModel(container);
            if (PlaceWindow(w, "RateWindow"))
            {
                w.Show();
            }
            container.RegisterInstance<RateWindow>(w);
        }

        private void NewVolatilityWindow(IUnityContainer container)
        {
            Thread t = new Thread(() =>
            {
                VolatilityWindow w = new VolatilityWindow(container);
                //w.DataContext = new VolatilityWindowViewModel(container, w.Dispatcher, exchanges);
                if (PlaceWindow(w, "VolatilityWindow"))
                {
                    w.Show();
                }

                container.RegisterInstance<VolatilityWindow>(w);
                w.Closed += (sender2, e2) => w.Dispatcher.InvokeShutdown();

                System.Windows.Threading.Dispatcher.Run();
            });
            t.SetApartmentState(ApartmentState.STA);
            //t.IsBackground = true;
            t.Start();
        }

        private void NewPortfolioWindow(IUnityContainer container)
        {
            Thread t = new Thread(() =>
            {
                PortfolioWindow w = new PortfolioWindow(container);
                w.DataContext = new PortfolioWindowViewModel(container, w.Dispatcher);
                if (PlaceWindow(w, "PortfolioWindow"))
                {
                    w.Show();
                }

                container.RegisterInstance<PortfolioWindow>(w);
                w.Closed += (sender2, e2) => w.Dispatcher.InvokeShutdown();

                System.Windows.Threading.Dispatcher.Run();
            });
            t.SetApartmentState(ApartmentState.STA);
            t.Start();
        }

        private void NewMessageWindow(IUnityContainer container)
        {
            Thread t = new Thread(() =>
            {
                MessageWindow w = new MessageWindow();
                w.DataContext = new MessageWindowViewModel(container, w.Dispatcher);
                if (PlaceWindow(w, "MessageWindow"))
                {
                    w.Show();
                }

                container.RegisterInstance<MessageWindow>(w);
                w.Closed += (sender2, e2) => w.Dispatcher.InvokeShutdown();

                System.Windows.Threading.Dispatcher.Run();
            });
            t.SetApartmentState(ApartmentState.STA);
            t.Start();
        }

        private void NewStrategyWindow(IUnityContainer container)
        {
            StrategyWindow w = new StrategyWindow(container);
            //w.DataContext = new StrategyWindowViewModel(container);
            if (PlaceWindow(w, "StrategyWindow"))
            {
                w.Show();
            }
            container.RegisterInstance<StrategyWindow>(w);
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            MainWindowViewModel vm = this.DataContext as MainWindowViewModel;
            vm.Stop();

            ///vm.Container.Resolve<EventAggregator>().GetEvent<CloseWindowEvent>().Publish();
            XmlWriter writer = null;
            try
            {
                XmlWriterSettings settings = new XmlWriterSettings();
                settings.Indent = true;
                writer = XmlWriter.Create(System.AppDomain.CurrentDomain.BaseDirectory + "\\layout\\" + Layout + "\\WindowPlacement.xml", settings);
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

                /// Close ExchangeWindow
                vm.Container.Resolve<ExchangeWindow>().SaveAndClose(writer);
                vm.Container.Resolve<RateWindow>().SaveAndClose(writer);
                vm.Container.Resolve<VolatilityWindow>().SaveAndClose(writer);
                vm.Container.Resolve<MessageWindow>().SaveAndClose(writer);
                vm.Container.Resolve<PortfolioWindow>().SaveAndClose(writer);
                vm.Container.Resolve<StrategyWindow>().SaveAndClose(writer);

                writer.WriteEndElement();
                writer.WriteEndDocument();
                writer.Flush();
            }
            catch (Exception) { }
            finally
            {
                if (writer != null) writer.Close();
            }
        }

        static public void WriteWindowPlacement(XmlWriter writer, Window window, string name)
        {
            writer.WriteStartElement("Window");
            writer.WriteAttributeString("Name", name);
            writer.WriteAttributeString("Visible", (window.Visibility == Visibility.Visible).ToString());
            writer.WriteAttributeString("Left", window.Left.ToString());
            writer.WriteAttributeString("Top", window.Top.ToString());
            writer.WriteAttributeString("Width", window.ActualWidth.ToString());
            writer.WriteAttributeString("Height", window.ActualHeight.ToString());
            writer.WriteEndElement();
            writer.Flush();
        }

        private bool PlaceWindow(Window window, string name)
        {
            Tuple<bool, double, double, double, double> placement = null;
            if (placements.TryGetValue(name, out placement))
            {
                window.Visibility = placement.Item1 ? Visibility.Visible : Visibility.Hidden;
                window.Left = placement.Item2;
                window.Top = placement.Item3;
                window.Width = placement.Item4;
                window.Height = placement.Item5;
                return placement.Item1;
            }
            return true;
        }

        static public void PlaceWindow(Window window, Tuple<bool, double, double, double, double> placement)
        {
            window.Visibility = placement.Item1 ? Visibility.Visible : Visibility.Hidden;
            window.Left = placement.Item2;
            window.Top = placement.Item3;
            window.Width = placement.Item4;
            window.Height = placement.Item5;
        }

        static public void LoadDataGridLayout(string name, DataGrid dg, Dictionary<int, string> formats)
        {
            XmlReader reader = null;
            try
            {
                reader = XmlReader.Create(System.AppDomain.CurrentDomain.BaseDirectory + "\\layout\\" + MainWindow.Layout + "\\datagrid\\" + name);
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
            catch (Exception) { }
            finally
            {
                if (reader != null) reader.Close();
            }
        }

        static public void SaveDataGridLayout(string name, DataGrid dg, Dictionary<int, string> formats)
        {
            XmlWriter writer = null;
            try
            {
                XmlWriterSettings settings = new XmlWriterSettings();
                settings.Indent = true;
                writer = XmlWriter.Create(System.AppDomain.CurrentDomain.BaseDirectory + "\\layout\\" + Layout + "\\datagrid\\" + name, settings);
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
            }
            catch (Exception) { }
            finally
            {
                if (writer != null) writer.Close();
            }
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

        private void ExchangesMenuItem_Click(object sender, RoutedEventArgs e)
        {
            ShowWindow<ExchangeWindow>();
        }

        private void RateMenuItem_Click(object sender, RoutedEventArgs e)
        {
            ShowWindow<RateWindow>();
        }

        private void VolatilityMenuItem_Click(object sender, RoutedEventArgs e)
        {
            ShowWindow<VolatilityWindow>();
        }

        private void MessagesMenuItem_Click(object sender, RoutedEventArgs e)
        {
            ShowWindow<MessageWindow>();
        }

        private void PortfoliosMenuItem_Click(object sender, RoutedEventArgs e)
        {
            ShowWindow<PortfolioWindow>();
        }

        private void StrategyMenuItem_Click(object sender, RoutedEventArgs e)
        {
            ShowWindow<StrategyWindow>();
        }

        private void MonitorMenuItem_Click(object sender, RoutedEventArgs e)
        {

        }

        private void CreditMenuItem_Click(object sender, RoutedEventArgs e)
        {

        }

        private void EmergencyMenuItem_Click(object sender, RoutedEventArgs e)
        {

        }

        private void ShowWindow<T>() where T : Window
        {
            MainWindowViewModel vm = this.DataContext as MainWindowViewModel;
            T w = vm.Container.Resolve<T>();
            Action action = () =>
            {
                if (w.IsVisible == false)
                {
                    w.Show();
                }
                w.Topmost = true;
                w.Topmost = false;
            };
            if (w.Dispatcher.CheckAccess())
            {
                action();
            }
            else
            {
                w.Dispatcher.Invoke(action);
            }
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
                    return Brushes.Green;
                case ConnectStatus.Disconnected:
                    return Brushes.Red;
                case ConnectStatus.Disabled:
                    return Brushes.Gray;
                default:
                    return Brushes.Gray;
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
