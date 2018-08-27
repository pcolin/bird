using client.Models;
using Microsoft.Practices.Unity;
using Prism.Commands;
using Prism.Events;
using Prism.Interactivity.InteractionRequest;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Security;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Input;
using System.Windows.Threading;
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
        public DelegateCommand<object> LoginCommand { get; set; }
        public ICommand LogoutCommand { get; set; }

        public InteractionRequest<INotification> NotificationRequest { get; set; }
        public InteractionRequest<IConfirmation> ConfirmationRequest { get; set; }

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
                    LoginCommand.RaiseCanExecuteChanged();
                }
            }
        }

        private bool isExchange1Logined;
        public bool IsExchange1Logined
        {
            get { return isExchange1Logined; }
            set { SetProperty(ref isExchange1Logined, value); }
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
                    LoginCommand.RaiseCanExecuteChanged();
                }
            }
        }

        private bool isExchange2Logined;
        public bool IsExchange2Logined
        {
            get { return isExchange2Logined; }
            set { SetProperty(ref isExchange2Logined, value); }
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
                    LoginCommand.RaiseCanExecuteChanged();
                }
            }
        }

        private bool isExchange3Logined;
        public bool IsExchange3Logined
        {
            get { return isExchange3Logined; }
            set { SetProperty(ref isExchange3Logined, value); }
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

        private bool isExchange4Logined;
        public bool IsExchange4Logined
        {
            get { return isExchange4Logined; }
            set { SetProperty(ref isExchange4Logined, value); }
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
                    LoginCommand.RaiseCanExecuteChanged();
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

        private DateTime time;
        public DateTime Time
        {
            get { return time; }
            set { SetProperty(ref time, value); }
        }        

        private string user = "colin";
        public string User
        {
            get { return user; }
            set { SetProperty(ref user, value); }
        }

        //private string password = "colin";
        //public string Password
        //{
        //    get { return password; }
        //    set { SetProperty(ref password, value); }
        //}

        private string layout;
        public string Layout
        {
            get { return layout; }
        }        
        
        public MainWindowViewModel(IUnityContainer container)
        {
            LoginCommand = new DelegateCommand<object>(new Action<object>(this.LoginExecute), new Func<object, bool>(this.CanLogin));
            //InitializeCommand = new DelegateCommand(this.InitializeExecute);
            //LoginCommand = new DelegateCommand(this.LoginExecute, this.CanLogin);
            LogoutCommand = new DelegateCommand(this.LogoutExecute, this.CanLogout);

            NotificationRequest = new InteractionRequest<INotification>();
            ConfirmationRequest = new InteractionRequest<IConfirmation>();
            //this.serverService = service;
            this.container = container;
            this.timer = new DispatcherTimer();
            this.timer.Interval = TimeSpan.FromSeconds(1);
            this.timer.Tick += new EventHandler(TimerTick);
            this.timer.Start();
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

            //this.container.RegisterInstance<EventAggregator>(new EventAggregator());
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
        
        public bool Stop()
        {
            bool success = true;
            if (IsExchange1Logined)
            {
                if (Logout(Exchange1) == false)
                    success = false;
            }
            if (IsExchange2Logined)
            {
                if (Logout(Exchange2) == false)
                    success = false;
            }
            if (IsExchange3Logined)
            {
                if (Logout(Exchange3) == false)
                    success = false;
            }
            if (IsExchange4Logined)
            {
                if (Logout(Exchange4) == false)
                    success = false;
            }
            return success;
        }

        //private void StopService(Proto.Exchange exchange)
        //{
        //    string exch = exchange.ToString();
        //    var server = this.container.Resolve<ServerService>(exch);
        //    if (server != null)
        //    {
        //        server.Stop();
        //    }
        //    var database = this.container.Resolve<DatabaseService>(exch);
        //    if (database != null)
        //    {
        //        database.Stop();
        //    }
        //    var proxy = this.container.Resolve<ProxyService>(exch);
        //    if (proxy != null)
        //    {
        //        proxy.Stop();
        //    }

        //    var calculator = this.container.Resolve<TheoCalculator>(exch);
        //    if (calculator != null)
        //    {
        //        calculator.Stop();
        //    }
        //}

        private void InitializeWindows()
        {

        }

        private void LoginExecute(object obj)
        {
            var pwd = obj as System.Windows.Controls.PasswordBox;
            List<Proto.Exchange> exchanges = new List<Proto.Exchange>();
            if (IsExchange1Selected)
            {
                if (Login(this.exchange1, pwd.Password))
                {
                    exchanges.Add(this.exchange1);
                    IsExchange1Logined = true;
                    Exchange1Status = ConnectStatus.Connected;
                    lastHeartbeatTimes[this.exchange1] = DateTime.Now;
                }
            }

            if (IsExchange2Selected)
            {
                if (Login(this.exchange2, pwd.Password))
                {
                    exchanges.Add(this.exchange2);
                    IsExchange2Logined = true;
                    Exchange2Status = ConnectStatus.Connected;
                    lastHeartbeatTimes[this.exchange2] = DateTime.Now;
                }
            }

            if (IsExchange3Selected)
            {
                if (Login(this.exchange3, pwd.Password))
                {
                    exchanges.Add(this.exchange3);
                    IsExchange3Logined = true;
                    Exchange3Status = ConnectStatus.Connected;
                    lastHeartbeatTimes[this.exchange3] = DateTime.Now;
                }
            }

            if (IsExchange4Selected)
            {
                if (Login(this.exchange4, pwd.Password))
                {
                    exchanges.Add(this.exchange4);
                    IsExchange4Logined = true;
                    Exchange4Status = ConnectStatus.Connected;
                    lastHeartbeatTimes[this.exchange4] = DateTime.Now;
                }
            }
            if (exchanges.Count > 0)
            {
                this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<List<Proto.Exchange>>>().Publish(exchanges);

                this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.Heartbeat>>().Subscribe(this.ReceiveHeartbeat, ThreadOption.BackgroundThread);

                //heartbeatTimer = new DispatcherTimer();
                //heartbeatTimer.Interval = this.interval;
                //heartbeatTimer.Tick += heartbeatTimer_Tick;
                //heartbeatTimer.Start();
            }
        }

        //public void Login(string password)
        //{
        //    List<Proto.Exchange> exchanges = new List<Proto.Exchange>();
        //    if (IsExchange1Selected)
        //    {
        //        if (Login(this.exchange1, password))
        //        {
        //            exchanges.Add(this.exchange1);
        //            Exchange1Status = ConnectStatus.Connected;
        //        }
        //    }

        //    if (IsExchange2Selected)
        //    {
        //        if (Login(this.exchange2, password))
        //        {
        //            exchanges.Add(this.exchange2);
        //            Exchange2Status = ConnectStatus.Connected;
        //        }
        //    }

        //    if (IsExchange3Selected)
        //    {
        //        if (Login(this.exchange3, password))
        //        {
        //            exchanges.Add(this.exchange3);
        //            Exchange3Status = ConnectStatus.Connected;
        //        }
        //    }

        //    if (IsExchange4Selected)
        //    {
        //        if (Login(this.exchange4, password))
        //        {
        //            exchanges.Add(this.exchange4);
        //            Exchange4Status = ConnectStatus.Connected;
        //        }
        //    }
        //    if (exchanges.Count > 0)
        //    {
        //        this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<List<Proto.Exchange>>>().Publish(exchanges);
        //    }
        //}

        private bool Login(Proto.Exchange exchange, string password)
        {
            string ex = exchange.ToString();
            var server = this.container.Resolve<ServerService>(ex);
            var r = server.Login(this.User, password, this.Version);
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

        //private bool Logout(Proto.Exchange exchange)
        //{
        //    string exch = exchange.ToString();
        //    var server = this.container.Resolve<ServerService>(exch);
        //    bool ret = true;
        //    Action<IConfirmation> action = c =>
        //        {
        //            if (c.Confirmed)
        //            {
        //                server.Stop();
        //                var database = this.container.Resolve<DatabaseService>(exch);
        //                if (database != null)
        //                {
        //                    database.Stop();
        //                }
        //                var proxy = this.container.Resolve<ProxyService>(exch);
        //                if (proxy != null)
        //                {
        //                    proxy.Stop();
        //                }

        //                var calculator = this.container.Resolve<TheoCalculator>(exch);
        //                if (calculator != null)
        //                {
        //                    calculator.Stop();
        //                }
        //            }
        //            else
        //            {
        //                ret = false;
        //            }
        //        };

        //    var r = server.Logout(this.User);
        //    if (r != null)
        //    {
        //        if (r.Result)
        //        {
        //            Exchange1Status = ConnectStatus.Disconnected;
        //            server.Stop();

        //            var database = this.container.Resolve<DatabaseService>(exch);
        //            if (database != null)
        //            {
        //                database.Stop();
        //            }
        //            var proxy = this.container.Resolve<ProxyService>(exch);
        //            if (proxy != null)
        //            {
        //                proxy.Stop();
        //            }

        //            var calculator = this.container.Resolve<TheoCalculator>(exch);
        //            if (calculator != null)
        //            {
        //                calculator.Stop();
        //            }
        //            return true;
        //        }
        //        else
        //        {
        //            NotificationRequest.Raise(new Notification { Content = r.Error, Title = "Login Failed" });
        //        }
        //    }
        //    else
        //    {
        //        //NotificationRequest.Raise(new Notification { Content = "Logout " + exchange + " timeout", Title = "Login Failed" });
        //        ConfirmationRequest.Raise(new Confirmation { Title = "Confirmation", Content = "Logout " + exchange + " timeout, exit anyway?" }, action);
        //    }
        //    return false;
        //}
        private bool Logout(Proto.Exchange exchange)
        {
            string exch = exchange.ToString();
            var server = this.container.Resolve<ServerService>(exch);
            bool logout = true;
            var r = server.Logout(this.User);
            if (r != null)
            {
                if (!r.Result)
                {
                    ConfirmationRequest.Raise(new Confirmation { Title = "Confirmation", Content = "Logout " + exchange + " failed: " + r.Error + ", exit anyway?" }, c => logout = c.Confirmed);
                }
            }
            else
            {
                ConfirmationRequest.Raise(new Confirmation { Title = "Confirmation", Content = "Logout " + exchange + " timeout, exit anyway?" }, c => logout = c.Confirmed);
            }
            if (logout)
            {
                server.Stop();
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
            return logout;
        }

        private bool CanLogin(Object obj)
        {
            return IsExchange1Selected || IsExchange2Selected || IsExchange3Selected || IsExchange4Selected;
        }

        private void LogoutExecute()
        {
            if (IsExchange1Logined)
            {
                Logout(this.exchange1);
            }

            if (IsExchange2Logined)
            {
                Logout(this.exchange2);
            }

            if (IsExchange3Logined)
            {
                Logout(this.exchange3);
            }

            if (IsExchange4Logined)
            {
                Logout(this.exchange4);
            }
        }

        private bool CanLogout()
        {
            return Exchange1Status == ConnectStatus.Connected || Exchange2Status == ConnectStatus.Connected ||
                   Exchange3Status == ConnectStatus.Connected || exchange4Status == ConnectStatus.Connected;
        }

        private void ReceiveHeartbeat(Proto.Heartbeat heartbeat)
        {
            if (heartbeat.Type == Proto.ProcessorType.Middleware && this.lastHeartbeatTimes.ContainsKey(heartbeat.Exchange))
            {
                this.lastHeartbeatTimes[heartbeat.Exchange] = DateTime.Now;
                //if (heartbeat.Exchange == this.Exchange1)
                //{
                //    this.Exchange1Status = ConnectStatus.Disconnected;
                //}
                //else if (heartbeat.Exchange == this.Exchange2)
                //{
                //    this.Exchange2Status = ConnectStatus.Disconnected;
                //}
                //else if (heartbeat.Exchange == this.Exchange3)
                //{
                //    this.Exchange3Status = ConnectStatus.Disconnected;
                //}
                //else if (heartbeat.Exchange == this.Exchange4)
                //{
                //    this.Exchange4Status = ConnectStatus.Disconnected;
                //}
            }
        }

        void TimerTick(object sender, EventArgs e)
        {
            this.Time = DateTime.Now;
            foreach (var kvp in lastHeartbeatTimes)
            {
                if (this.time - kvp.Value > interval)
                {
                    var err = new Proto.ServerInfo()
                    {
                        Time = (ulong)((DateTime.Now - new DateTime(1970, 1, 1, 8, 0, 0)).TotalSeconds),
                        Type = Proto.InfoType.Warn,
                        Exchange = kvp.Key,
                        Info = "Server heartbeat lost for " + this.interval.TotalSeconds + 's',
                    };
                    if (kvp.Key == this.Exchange1)
                    {
                        if (this.Exchange1Status == ConnectStatus.Connected)
                        {
                            this.Exchange1Status = ConnectStatus.Disconnected;
                            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.ServerInfo>>().Publish(err);
                        }
                    }
                    else if (kvp.Key == this.Exchange2)
                    {
                        if (this.Exchange2Status == ConnectStatus.Connected)
                        {
                            this.Exchange2Status = ConnectStatus.Disconnected;
                            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.ServerInfo>>().Publish(err);
                        }
                    }
                    else if (kvp.Key == this.Exchange3)
                    {
                        if (this.Exchange3Status == ConnectStatus.Connected)
                        {
                            this.Exchange3Status = ConnectStatus.Disconnected;
                            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.ServerInfo>>().Publish(err);
                        }
                    }
                    else if (kvp.Key == this.Exchange4)
                    {
                        if (this.Exchange4Status == ConnectStatus.Connected)
                        {
                            this.Exchange4Status = ConnectStatus.Disconnected;
                            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.ServerInfo>>().Publish(err);
                        }
                    }
                }
            }
        }

        //void heartbeatTimer_Tick(object sender, EventArgs e)
        //{
        //    //throw new NotImplementedException();
        //    var now = DateTime.Now;
        //    foreach (var kvp in lastHeartbeatTimes)
        //    {
        //        if (now - kvp.Value > interval)
        //        {
        //            if (kvp.Key == this.Exchange1)
        //            {
        //                this.Exchange1Status = ConnectStatus.Disconnected;
        //            }
        //            else if (kvp.Key == this.Exchange2)
        //            {
        //                this.Exchange2Status = ConnectStatus.Disconnected;
        //            }
        //            else if (kvp.Key == this.Exchange3)
        //            {
        //                this.Exchange3Status = ConnectStatus.Disconnected;
        //            }
        //            else if (kvp.Key == this.Exchange4)
        //            {
        //                this.Exchange4Status = ConnectStatus.Disconnected;
        //            }
        //        }
        //    }
        //}

        public IUnityContainer Container
        {
            get { return this.container; }
        }
        IUnityContainer container;

        DispatcherTimer timer;
        //DispatcherTimer heartbeatTimer;
        TimeSpan interval = TimeSpan.FromSeconds(10);
        Dictionary<Proto.Exchange, DateTime> lastHeartbeatTimes = new Dictionary<Proto.Exchange,DateTime>();
        //private Dictionary<Exchange, ServerService> servers;
        //private Dictionary<Exchange, ProxyService> proxies;
        //private Dictionary<Exchange, DatabaseService> databases;
    }
}
