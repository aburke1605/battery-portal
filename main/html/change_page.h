static const char change_html[] = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <!DOCTYPE HTML><html>

<html>
<head>
<meta name='viewport' content='width=device-width, initial-scale=1'>
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
    <center> <h1> Change battery properties </h1> </center>

        <div class='container'>
          <form action='/validate_change' method='post'>
            <label>Update BL voltage threshold (currently <span id='BL'></span> mV): </label>
            <input type='text' placeholder='Enter value (mV)' name='BL_voltage_threshold'>
            <label>Update BH voltage threshold (currently <span id='BH'></span> mV): </label>
            <input type='text' placeholder='Enter value (mV)' name='BH_voltage_threshold'>
            <label>Update charge current threshold (currently <span id='CCT'></span> mA): </label>
            <input type='text' placeholder='Enter value (mA)' name='charge_current_threshold'>
            <label>Update discharge current threshold (currently <span id='DCT'></span> mA): </label>
            <input type='text' placeholder='Enter value (mA)' name='discharge_current_threshold'>
            <button type='submit'>Submit</button>
          </form>
        </div>

  <script>
    let socket = new WebSocket('ws://' + location.host + '/ws');
    socket.onopen = function() {
        console.log('WebSocket connection established');
    };
    socket.onmessage = function(event) {
        try {
            let data = JSON.parse(event.data);
            document.getElementById('BL').innerHTML = data.BL;
            document.getElementById('BH').innerHTML = data.BH;
            document.getElementById('CCT').innerHTML = data.CCT;
            document.getElementById('DCT').innerHTML = data.DCT;
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
