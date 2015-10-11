var locationOptions = {
  enableHighAccuracy: true, 
  maximumAge: 10000, 
  timeout: 10000
};

function locationSuccess(pos) {
  var d = new Date();
  var tz = d.getTimezoneOffset()*60;
  console.log('lat= ' + pos.coords.latitude + ' lon= ' + pos.coords.longitude + ' tz= '+tz);
  var thing = { '1': Math.round(pos.coords.longitude * (12/180) *60*60*100) , '2': tz };
  Pebble.sendAppMessage( thing,
    function(e) {
      console.log("SUCCEED!");
      console.log('Successfully delivered message with transactionId=' + e.data.transactionId);
    },
    function(e) {
      console.log("FAIL");
      console.log('Unable to deliver message with transactionId='+ e.data.transactionId + ' Error is: ' + e.error.message);
    }
  ); 
  console.log("Done!");
  for (var key in thing) {
    console.log(key + " " + thing[key]);
  }
}

function locationError(err) {
  console.log('location error (' + err.code + '): ' + err.message);
}

Pebble.addEventListener('ready',
  function(e) {
    // Request current position
    navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
  }
);

