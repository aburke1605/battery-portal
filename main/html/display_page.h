static const char display_html[] = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <!DOCTYPE HTML><html>

<html>
<head>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<title> Info Page </title>
<link rel='icon' href='data:,'>
<style>
Body {
  font-family: Calibri, Helvetica, sans-serif;
  background-color: lightblue;
}


button {
       background-color: #4CAF50;
       width: 100%;
        color: orange;
        padding: 15px;
        margin: 30px 0px;
        border: none;
        cursor: pointer;
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
        -webkit-tap-highlight-color: rgba(0,0,0,0);
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
 .button:active {
     background-color: #0f8b8d;
     box-shadow: 2 2px #CDCDCD;
     transform: translateY(2px);
   }
   .state {
     font-size: 1.5rem;
     color:#8c8c8c;
     font-weight: bold;
   }
  .cancelbtn {
        width: auto;
        padding: 10px 18px;
        margin: 10px 5px;
    }


 .container {
        padding: 25px;
        background-color: lightgreen;
    }


</style>
<link rel='icon' href='data:,'>
</head>
<body>
    <center> <h1> Battery information </h1> </center>

        <div class='container'>
          <p>
            <span class='charge'>State of charge:</span>
            <span id='charge'>%CHARGE%</span>
            <span id='charge'>&#37</span>
          </p>
          <p>
            <span class='voltage'>Voltage:</span>
            <span id='voltage'>%VOLTAGE%</span>
            <span id='voltage'>mV</span>
          </p>
          <p>
            <span class='current'>Current:</span>
            <span id='current'>%CURRENT%</span>
            <span id='current'>mA</span>
          </p>
          <p>
            <span class='temp'>Temperature:</span>
            <span id='temp'>%TEMPERATURE%</span>
            <span id='temp'>degC</span>
          </p>
        </div>
        <p><button id='bluebutton' class='button'>Blue</button>
          <span class='state'>Blue LED:</span>
          <span id='bluestate'>%STATE%</span>
        </p>
        <p><button id='redbutton' class='button'>Red</button>
          <span class='state'>Red LED:</span>
          <span id='redstate'>%STATE%</span>
        </p>
        <img src='/image/aceon2.png' style='max-width: 100%; height:auto;'>
        <form action='/about' method='get'>
          <button type='Submit'>About AceOn</button>
        </form>
        <form action='/device' method='get'>
          <button type='Submit'>Unit layout</button>
        </form>

  <h1>Live Timer</h1>
  <p>(proof the websocket is connected)</p>
  <div id='time'>Connecting...</div>
  <script>
    let socket = new WebSocket('ws://' + location.host + '/ws');
    socket.onopen = function() {
        console.log('WebSocket connection established');
    };
    socket.onmessage = function(event) {
        document.getElementById('time').textContent = event.data;
    };
    socket.onerror = function(error) {
        console.error('WebSocket error:', error);
    };
    socket.onclose = function() {
        console.log('WebSocket connection closed');
    };
  </script>

</body>
</html>
)rawliteral";
