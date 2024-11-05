const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <!DOCTYPE HTML><html>
  
<html>   
<head>  
<meta name="viewport" content="width=device-width, initial-scale=1">  
<title> Login Page </title>  
<style>   
Body {  
  font-family: Calibri, Helvetica, sans-serif;  
  background-color: pink;  
}  
button {   
       background-color: #4CAF50;   
       width: 100%;  
        color: white;   
        padding: 15px;   
        margin: 10px 0px;   
        border: none;   
        cursor: pointer;   
         }   
 form {   
        border: 3px solid #f1f1f1;   
    }   
 input[type=text], input[type=password] {   
        width: 100%;   
        margin: 8px 0;  
        padding: 12px 20px;   
        display: inline-block;   
        border: 2px solid green;   
        box-sizing: border-box;   
    }  
 button:hover {   
        opacity: 0.7;   
    }   
  .cancelbtn {   
        width: auto;   
        padding: 10px 18px;  
        margin: 10px 5px;  
    }   
        
     
 .container {   
        padding: 25px;   
        background-color: lightblue;  
    }

  img {
    height: auto; 
    max-width: 100%;
  }
       
</style>   
</head>    
<body>    
    <center> <h1> Login Form </h1> </center>   
    
    <div class="container">   
        <form action="/get">   
            <label>Username : </label>   
            <input type="text" placeholder="Enter Username" name="username" required>  
            <label>Password : </label>   
            <input type="password" placeholder="Enter Password" name="password" required>  
            <button type="submit">Submit</button> 
            <input type="checkbox" checked="checked"> Remember me   
            <button type="button" class="cancelbtn"> Cancel</button>   
            Forgot <a href="#"> password? </a>  
        </form> 
        <p>Temp: <span id="Charge">CHARGE</span> &#37</p>
        <p>Temp: <span id="Voltage">VOLTAGE</span> mV</p>
        <p>Temp: <span id="Current">CURRENT</span> mV</p>
        <p>Temp: <span id="Temperature">TEMPERATURE</span> &degC</p>
    </div>   
    <img src="/aceon">
        
    <script>
        var gateway = `ws://${window.location.hostname}/ws`;
        var websocket;
        window.addEventListener('load', onLoad);
        function initWebSocket() {
            console.log('Trying to open a WebSocket connection...');
            websocket = new WebSocket(gateway);
            websocket.onopen    = onOpen;
            websocket.onclose   = onClose;
            websocket.onmessage = onMessage; // <-- add this line
        }
        function onOpen(event) {
            console.log('Connection opened');
        }
        function onClose(event) {
            console.log('Connection closed'); 
        }
        function onMessage(event) {
            var info = JSON.parse(event.data);    //extract individual data from json string
            var charge = info["Q"];
            var voltage = info["V"];          //put each data type into javascript variable 
            var current = info["I"];
            var temperature = info["T"];
            
            document.getElementById('Charge').innerHTML = charge;
            document.getElementById('Voltage').innerHTML = voltage;
            document.getElementById('Current').innerHTML = current;
            document.getElementById('Temperature').innerHTML = temperature;
        }
        function onLoad(event) {
            initWebSocket();
        }
    </script>
        
</body>     
</html>  
</html>)rawliteral";