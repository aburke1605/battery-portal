static const char nearby_html[] = R"rawliteral(
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
          <p>State of charge: <span id='charge'></span> %</p>
          <p>Voltage: <span id='voltage'></span> V</p>
          <p>Current: <span id='current'></span> A</p>
          <p>Temperature: <span id='temperature'></span> degC</p>
        </div>

        <img src='/image/aceon2.png' style='max-width: 100%; height:auto;'>

  <script>
    let socket = new WebSocket('ws://' + location.host + '/ws');
    socket.onopen = function() {
        console.log('WebSocket connection established');
    };
    socket.onmessage = function(event) {
        try {
            let data = JSON.parse(event.data);
            document.getElementById('charge').innerHTML = data.received_data.charge;
            document.getElementById('voltage').innerHTML = data.received_data.voltage.toFixed(2);
            document.getElementById('current').innerHTML = data.received_data.current.toFixed(2);
            document.getElementById('temperature').innerHTML = data.received_data.temperature.toFixed(2);
        } catch (error) {
            console.error('Error parsing JSON:', error);
        }
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
