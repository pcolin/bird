using client.Models;
using Microsoft.Practices.Unity;
using Prism.Commands;
using Prism.Events;
using Prism.Interactivity.InteractionRequest;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Input;
using System.Xml;

namespace client.ViewModels
{
    public enum ConnectStatus
    {
        Connected = 0,
        Disconnected = 1,
        Disabled = 2,
    };

    class MainWindowViewModel : BindableBase
    {
        public ICommand InitializeCommand { get; set; }
        public ICommand LoginCommand { get; set; }
        public ICommand LogoutCommand { get; set; }

        public InteractionRequest<INotification> NotificationRequest { get; set; }

        private string version = "0.2.01";
        public string Version
        {
            get { return version; }
            //set { SetProperty(ref version, value); }
        }

        public string Title
        {
            get { return "Bird " + Version; }
        }

        private Proto.Exchange exchange1;
        public Proto.Exchange Exchange1
        {
            get { return exchange1; }
            set { SetProperty(ref this.exchange1, value); }
        }

        private bool exchange1Visible = false;
        public bool Exchange1Visible
        {
            get { return exchange1Visible; }
            set { SetProperty(ref exchange1Visible, value); }
        }        

        private bool isExchange1Selected = true;
        public bool IsExchange1Selected
        {
            get { return isExchange1Selected; }
            set
            {
                if (SetProperty(ref isExchange1Selected, value))
                {
                    (LoginCommand as DelegateCommand).RaiseCanExecuteChanged();
                }
            }
        }        

        private ConnectStatus exchange1Status = ConnectStatus.Disabled;
        public ConnectStatus Exchange1Status
        {
            get { return exchange1Status; }
            set
            {
                if (SetProperty(ref exchange1Status, value))
                {
                    (LogoutCommand as DelegateCommand).RaiseCanExecuteChanged();
                }
            }
        }        

        private Proto.Exchange exchange2;
        public Proto.Exchange Exchange2
        {
            get { return exchange2; }
            set { SetProperty(ref exchange2, value); }
        }

        private bool exchange2Visible = false;
        public bool Exchange2Visible
        {
            get { return exchange2Visible; }
            set { SetProperty(ref exchange2Visible, value); }
        }

        private bool isExchange2Selected;
        public bool IsExchange2Selected
        {
            get { return isExchange2Selected; }
            set
            {
                if (SetProperty(ref isExchange2Selected, value))
                {
                    (LoginCommand as DelegateCommand).RaiseCanExecuteChanged();
                }
            }
        }

        private ConnectStatus exchange2Status = ConnectStatus.Disabled;
        public ConnectStatus Exchange2Status
        {
            get { return exchange2Status; }
            set
            {
                if (SetProperty(ref exchange2Status, value))
                {
                    (LogoutCommand as DelegateCommand).RaiseCanExecuteChanged();
                }
            }
        }

        private Proto.Exchange exchange3;
        public Proto.Exchange Exchange3
        {
            get { return exchange3; }
            set { SetProperty(ref exchange3, value); }
        }

        private bool exchange3Visible = false;
        public bool Exchange3Visible
        {
            get { return exchange3Visible; }
            set { SetProperty(ref exchange3Visible, value); }
        }

        private bool isExchange3Selected;
        public bool IsExchange3Selected
        {
            get { return isExchange3Selected; }
            set
            {
                if (SetProperty(ref isExchange3Selected, value))
                {
                    (LoginCommand as DelegateCommand).RaiseCanExecuteChanged();
                }
            }
        }

        private ConnectStatus exchange3Status = ConnectStatus.Disabled;
        public ConnectStatus Exchange3Status
        {
            get { return exchange3Status; }
            set
            {
                if (SetProperty(ref exchange3Status, value))
                {
                    (LogoutCommand as DelegateCommand).RaiseCanExecuteChanged();
                }
            }
        }

        private Proto.Exchange exchange4;
        public Proto.Exchange Exchange4
        {
            get { return exchange4; }
            set { SetProperty(ref this.exchange4, value); }
        }


        private bool exchange4Visible = false;
        public bool Exchange4Visible
        {
            get { return exchange4Visible; }
            set { SetProperty(ref exchange4Visible, value); }
        }

        private bool isExchange4Selected;
        public bool IsExchange4Selected
        {
            get { return isExchange4Selected; }
            set
            {
                if (SetProperty(ref isExchange4Selected, value))
                {
                    (LoginCommand as DelegateCommand).RaiseCanExecuteChanged();
                }
            }
        }

        private ConnectStatus exchange4Status = ConnectStatus.Disabled;
        public ConnectStatus Exchange4Status
        {
            get { return exchange4Status; }
            set
            {
                if (SetProperty(ref exchange4Status, value))
                {
                    (LogoutCommand as DelegateCommand).RaiseCanExecuteChanged();
                }
            }
        }

        private string user = "colin";
        public string User
        {
            get { return user; }
            set { SetProperty(ref user, value); }
        }

        private string password = "colin";
        public string Password
        {
            get { return password; }
            set { SetProperty(ref password, value); }
        }

        private string layout;
        public string Layout
        {
            get { return layout; }
        }        
        
        public MainWindowViewModel(IUnityContainer container)
        {
            //LoginCommand = new DelegateCommand<object>(new Action<object>(this.LoginExecute), new Func<object, bool>(this.CanLogin));
            //InitializeCommand = new DelegateCommand(this.InitializeExecute);
            LoginCommand = new DelegateCommand(this.LoginExecute, this.CanLogin);
            LogoutCommand = new DelegateCommand(this.LogoutExecute, this.CanLogout);

            NotificationRequest = new InteractionRequest<INotification>();
            //this.serverService = service;
            this.container = container;
        }

        //private void InitializeExecute()
        public void Initialize()
        {
            /// Load config from xml
            try
            {
                XmlDocument doc = new XmlDocument();
                doc.Load(System.AppDomain.CurrentDomain.BaseDirectory + "\\config\\config.xml");
                XmlElement root = doc.DocumentElement;
                this.user = root.GetAttribute("user");
                this.layout = root.GetAttribute("layout");

                XmlNode node = root.FirstChild;
                XmlElement element = (XmlElement)node;
                string name = element.GetAttribute("name");
                Proto.Exchange exchange;
                if (Enum.TryParse<Proto.Exchange>(name, true, out exchange) == false)
                {
                    NotificationRequest.Raise(new Notification { Content = "Unkown exchange " + name, Title = "Error" });
                    return;
                }
                this.Exchange1 = exchange;
                this.Exchange1Visible = true;
                InitializeService(this.exchange1, element.GetElementsByTagName("Server")[0].InnerText,
                    element.GetElementsByTagName("Database")[0].InnerText, element.GetElementsByTagName("Proxy")[0].InnerText);

                node = node.NextSibling;
                if (node == null)
                {
                    return;
                }
                element = (XmlElement)node;
                name = element.GetAttribute("name");
                if (Enum.TryParse<Proto.Exchange>(name, true, out exchange) == false)
                {
                    NotificationRequest.Raise(new Notification { Content = "Unkown exchange " + name, Title = "Error" });
                    return;
                }
                this.Exchange2 = exchange;
                this.Exchange2Visible = true;
                InitializeService(this.exchange2, element.GetElementsByTagName("Server")[0].InnerText,
                    element.GetElementsByTagName("Database")[0].InnerText, element.GetElementsByTagName("Proxy")[0].InnerText);

                node = node.NextSibling;
                if (node == null)
                {
                    return;
                }
                element = (XmlElement)node;
                name = element.GetAttribute("name");
                if (Enum.TryParse<Proto.Exchange>(name, true, out exchange) == false)
                {
                    NotificationRequest.Raise(new Notification { Content = "Unkown exchange " + name, Title = "Error" });
                    return;
                }
                this.Exchange3 = exchange;
                this.Exchange3Visible = true;
                InitializeService(this.exchange3, element.GetElementsByTagName("Server")[0].InnerText,
                    element.GetElementsByTagName("Database")[0].InnerText, element.GetElementsByTagName("Proxy")[0].InnerText);


                node = node.NextSibling;
                if (node == null)
                {
                    return;
                }
                element = (XmlElement)node;
                name = element.GetAttribute("name");
                if (Enum.TryParse<Proto.Exchange>(name, true, out exchange) == false)
                {
                    NotificationRequest.Raise(new Notification { Content = "Unkown exchange " + name, Title = "Error" });
                    return;
                }
                this.Exchange4 = exchange;
                this.Exchange4Visible = true;
                InitializeService(this.exchange4, element.GetElementsByTagName("Server")[0].InnerText,
                    element.GetElementsByTagName("Database")[0].InnerText, element.GetElementsByTagName("Proxy")[0].InnerText);
            }
            catch (Exception e)
            {
                NotificationRequest.Raise(new Notification { Content = "Load config exception: " + e.Message, Title = "Error" });
            }

            this.container.RegisterInstance<EventAggregator>(new EventAggregator());

            //InitializeWindows();
        }

        private void InitializeService(Proto.Exchange exchange, string serverAddress, string databaseAddress, string proxyAddress)
        {
            var server = new ServerService();
            server.Initialize(serverAddress);
            this.container.RegisterInstance<ServerService>(exchange.ToString(), server);

            var database = new DatabaseService(this.container, exchange, this.User);
            database.Initialize(databaseAddress);
            this.container.RegisterInstance<DatabaseService>(exchange.ToString(), database);

            var proxy = new ProxyService(this.container, exchange);
            proxy.Initialize(proxyAddress);
            this.container.RegisterInstance<ProxyService>(exchange.ToString(), proxy);

            var calculator = new TheoCalculator(this.container, exchange);
            this.container.RegisterInstance<TheoCalculator>(exchange.ToString(), calculator);
        }
        
        public void Stop()
        {
            if (Exchange1Visible)
            {
                StopService(Exchange1);
            }
            if (Exchange2Visible)
            {
                StopService(Exchange2);
            }
            if (Exchange3Visible)
            {
                StopService(Exchange3);
            }
            if (Exchange4Visible)
            {
                StopService(Exchange4);
            }
        }

        private void StopService(Proto.Exchange exchange)
        {
            string exch = exchange.ToString();
            var server = this.container.Resolve<ServerService>(exch);
            if (server != null)
            {
                server.Stop();
            }
            var database = this.container.Resolve<DatabaseService>(exch);
            if (database != null)
            {
                database.Stop();
            }
            var proxy = this.container.Resolve<ProxyService>(exch);
            if (proxy != null)
            {
                proxy.Stop();
            }

            var calculator = this.container.Resolve<TheoCalculator>(exch);
            if (calculator != null)
            {
                calculator.Stop();
            }
        }

        private void InitializeWindows()
        {

        }
       
        private void LoginExecute()
        {
            List<Proto.Exchange> exchanges = new List<Proto.Exchange>();
            if (IsExchange1Selected)
            {
                if (Login(this.exchange1))
                {
                    exchanges.Add(this.exchange1);
                    Exchange1Status = ConnectStatus.Connected;
                }
            }

            if (IsExchange2Selected)
            {
                if (Login(this.exchange2))
                {
                    exchanges.Add(this.exchange2);
                    Exchange2Status = ConnectStatus.Connected;
                }
            }

            if (IsExchange3Selected)
            {
                if (Login(this.exchange3))
                {
                    exchanges.Add(this.exchange3);
                    Exchange3Status = ConnectStatus.Connected;
                }
            }

            if (IsExchange4Selected)
            {
                if (Login(this.exchange4))
                {
                    exchanges.Add(this.exchange4);
                    Exchange4Status = ConnectStatus.Connected;
                }
            }
            if (exchanges.Count > 0)
            {
                this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<List<Proto.Exchange>>>().Publish(exchanges);
            }
        }

        private bool Login(Proto.Exchange exchange)
        {
            string ex = exchange.ToString();
            var server = this.container.Resolve<ServerService>(ex);
            var r = server.Login(this.User, this.Password, this.Version);
            if (r != null)
            {
                if (r.Result)
                {
                    
                    server.Start();
                    var database = this.container.Resolve<DatabaseService>(ex);
                    database.Start();
                    var proxy = this.container.Resolve<ProxyService>(ex);
                    proxy.Start();

                    var calculator = this.container.Resolve<TheoCalculator>(ex);
                    calculator.Start();
                    return true;
                }
                else
                {
                    NotificationRequest.Raise(new Notification { Content = r.Error, Title = "Login Failed" });
                }
            }
            else
            {
                NotificationRequest.Raise(new Notification { Content = "Login " + exchange + " timeout", Title = "Login Failed" });
            }
            return false;
        }

        private void Logout(Proto.Exchange exchange)
        {
            var server = this.container.Resolve<ServerService>(exchange.ToString());
            var r = server.Logout(this.User);
            if (r != null)
            {
                if (r.Result)
                {
                    Exchange1Status = ConnectStatus.Disconnected;
                    server.Stop();
                }
                else
                {
                    NotificationRequest.Raise(new Notification { Content = r.Error, Title = "Login Failed" });
                }
            }
            else
            {
                NotificationRequest.Raise(new Notification { Content = "Logout " + exchange + " timeout", Title = "Login Failed" });
            }
        }

        private bool CanLogin()
        {
            return IsExchange1Selected || IsExchange2Selected || IsExchange3Selected || IsExchange4Selected;
        }

        private void LogoutExecute()
        {
            if (IsExchange1Selected)
            {
                Logout(this.exchange1);
            }

            if (IsExchange2Selected)
            {
                Logout(this.exchange2);
            }

            if (IsExchange3Selected)
            {
                Logout(this.exchange3);
            }

            if (IsExchange4Selected)
            {
                Logout(this.exchange4);
            }
        }

        private bool CanLogout()
        {
            return Exchange1Status == ConnectStatus.Connected || Exchange2Status == ConnectStatus.Connected ||
                   Exchange3Status == ConnectStatus.Connected || exchange4Status == ConnectStatus.Connected;
        }

        //private bool TryParse(string name, out Proto.Exchange exchange)
        //{
        //    //if (name == "DCE")
        //    //{
        //    //    exchange = Proto.Exchange.De;
        //    //}
        //    //else if (name == "CZCE")
        //    //{
        //    //    exchange = Proto.Exchange.Ze;
        //    //}
        //    //else if (name == "SH)
        //    return false;
        //}

        public IUnityContainer Container
        {
            get { return this.container; }
        }
        IUnityContainer container;
        //private Dictionary<Exchange, ServerService> servers;
        //private Dictionary<Exchange, ProxyService> proxies;
        //private Dictionary<Exchange, DatabaseService> databases;
    }
}
