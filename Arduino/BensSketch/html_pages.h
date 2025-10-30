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
        </div>
        <img src="aceon">

</body>
</html>
</html>)rawliteral";





const char display_page[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <!DOCTYPE HTML><html>

<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title> Info Page </title>
<link rel="icon" href="data:,">
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
<link rel="icon" href="data:,">
</head>
<body>
    <center> <h1> Battery information </h1> </center>

        <div class="container">
          <p>
            <span class="charge">State of charge:</span>
            <span id="charge">%CHARGE%</span>
            <span id="charge">&#37</span>
          </p>
          <p>
            <span class="voltage">Voltage:</span>
            <span id="voltage">%VOLTAGE%</span>
            <span id="voltage">mV</span>
          </p>
          <p>
            <span class="current">Current:</span>
            <span id="current">%CURRENT%</span>
            <span id="current">mA</span>
          </p>
          <p>
            <span class="temp">Temperature:</span>
            <span id="temp">%TEMPERATURE%</span>
            <span id="temp">degC</span>
          </p>
        </div>
        <p><button id="bluebutton" class="button">Blue</button>
          <span class="state">Blue LED:</span>
          <span id="bluestate">%STATE%</span>
        </p>
        <p><button id="redbutton" class="button">Red</button>
          <span class="state">Red LED:</span>
          <span id="redstate">%STATE%</span>
        </p>
        <img src="aceon2" style="max-width: 100%; height:auto;">
        <form action="/about">
          <button type="Submit">About AceOn</button>
        </form>
        <form action="/unit image">
          <button type="Submit">Unit layout</button>
        </form>
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

    var bluestate;
    var redstate;
    var SOC;
    var voltage;
    var current;
    var temp;
    var info = 0;
    if (event.data == "1"){
      bluestate = "ON";
      document.getElementById('bluestate').innerHTML = bluestate;
    }
    else if(event.data == "0"){
      bluestate = "OFF";
      document.getElementById('bluestate').innerHTML = bluestate;
    }
    else if(event.data == "2"){
      redstate = "OFF";
      document.getElementById('redstate').innerHTML = redstate;
    }
    else if(event.data == "3"){
      redstate = "ON";
      document.getElementById('redstate').innerHTML = redstate;
    }

    else{

       info = JSON.parse(event.data);    //extract individual data from json string
       SOC = info["charge"];
       voltage = info["volts"];          //put each data type into javascript variable
       current = info["amps"];
       temp = info["temp"];

       document.getElementById('charge').innerHTML = SOC;
       document.getElementById('voltage').innerHTML = voltage;    //replace values on page with new data values
       document.getElementById('current').innerHTML = current;
       document.getElementById('temp').innerHTML = temp;
       }
  }
  function onLoad(event) {
    initWebSocket();
    initButton();
  }
  function initButton() {
    document.getElementById('bluebutton').addEventListener('click', toggleblue);
    document.getElementById('redbutton').addEventListener('click', togglered);
  }
  function toggleblue(){
    websocket.send('toggleblue');
  }
  function togglered(){
    websocket.send('togglered');
  }
</script>
</body>
</html>
</html>)rawliteral";





const char about_aceon[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <!DOCTYPE HTML><html>

<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title> About AceOn </title>
<style>
Body {
  font-family: Calibri, Helvetica, sans-serif;
  background-color: lightgreen;
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

 .container {
        padding: 25px;
        background-color: lightblue;
    }
 .font-style{
        font-weight:bold;
 }

  img {
    height: auto;
    max-width: 100%;
  }

</style>
</head>
<body>
    <center> <h1> About AceOn </h1> </center>

        <div class="container">
          <form action="/return">
           <p><span class="font-style">Address:</span></p>
           <p>Unit 9B<br>Stafford Park 12<br>Telford<br>Shropshire<br>TF3 3BJ</p>
           <p><span class="font-style">Telephone: </span>01952 293388</p>
           <a href="https://www.aceongroup.com">Our website</a>
           <p> </p>
           <button type="Submit">Back</button>
          </form>
        </div>

</body>
</html>
</html>)rawliteral";




const char device_image[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <!DOCTYPE HTML><html>

<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title> Unit layout </title>
<style>
Body {
  font-family: Calibri, Helvetica, sans-serif;
  background-color: lightgreen;
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

 .container {
        padding: 25px;
        background-color: lightblue;
    }
 .font-style{
        font-weight:bold;
 }

  img {
    height: auto;
    max-width: 100%;
  }

</style>
</head>
<body>
    <center> <h1> Unit layout </h1> </center>
         <p>PUT IMAGE OF UNIT HERE</p>
         <form action="/return">
            <button type="Submit">Back</button>
         </form>
</body>
</html>
</html>)rawliteral";
