var getFromBetween = {
    results:[],
    string:"",
    getFromBetween:function (sub1,sub2) {
        if(this.string.indexOf(sub1) < 0 || this.string.indexOf(sub2) < 0) return false;
        var SP = this.string.indexOf(sub1)+sub1.length;
        var string1 = this.string.substr(0,SP);
        var string2 = this.string.substr(SP);
        var TP = string1.length + string2.indexOf(sub2);
        return this.string.substring(SP,TP);
    },
    removeFromBetween:function (sub1,sub2) {
        if(this.string.indexOf(sub1) < 0 || this.string.indexOf(sub2) < 0) return false;
        var removal = sub1+this.getFromBetween(sub1,sub2)+sub2;
        this.string = this.string.replace(removal,"");
    },
    getAllResults:function (sub1,sub2) {
        // first check to see if we do have both substrings
        if(this.string.indexOf(sub1) < 0 || this.string.indexOf(sub2) < 0) return;

        // find one result
        var result = this.getFromBetween(sub1,sub2);
        // push it to the results array
        this.results.push(result);
        // remove the most recently found one from the string
        this.removeFromBetween(sub1,sub2);

        // if there's more substrings
        if(this.string.indexOf(sub1) > -1 && this.string.indexOf(sub2) > -1) {
            this.getAllResults(sub1,sub2);
        }
        else return;
    },
    get:function (string,sub1,sub2) {
        this.results = [];
        this.string = string;
        this.getAllResults(sub1,sub2);
        return this.results;
    }
};
function send_comand(chip_number) {
  var xhttp = new XMLHttpRequest;
  xhttp.open("POST", "/", true);
  xhttp.send("action=erasen&chip1=" + chip_number);
  // window.location = window.location;
}
function prepare_response() {
	var all_get = decodeURIComponent(window.location.search);
	var str = all_get.replace("?r=","");
	if(!str) return;
	if (str.indexOf('<') > -1) {
		// alert(str);
		var chip_array = getFromBetween.get(str,"<",">");
		// alert(chip_array);
		var link = "/?action=erasen&chip1=";
		var container = document.getElementById("divTable");
		container.style.display = 'table';
		for (i = 0; i < chip_array.length; i++) { 
			var divrow = document.createElement("div");
				divrow.setAttribute('class', 'divRow');
				var divcell = document.createElement("div");
					divcell.setAttribute('class', 'divCell');
					divcell.innerHTML = chip_array[i] + " ";
					var a = document.createElement("a");
					a.setAttribute('href', link + chip_array[i] );
					//a.setAttribute('onclick', 'send_comand(' + chip_array[i] + ')' );
						a.innerHTML = "<img src=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAB7klEQVQ4jY2Tz2vTYBjHv4eAgbjZtazppMncqNtkh25gxxxrspB1qaOK9SIM8WQ9+wfsb5iXMmolDvQP0IMguptzsMPofsFKo+3qQVC7VHvOC88Os11SUvGBz+n5fj+H9+EF/k4eCG1pN2vmZV5Dj9kQhNRnPVFdA4KeRR4I7WcUu7Wao5PH95ifZEMQUvVclrVWc1TKzDc6kjwQ2jPm7NbTR9SmtrLMTP5C8koQjPrDDHNnSouz55Kt6cla68kD6qaWTTGT57XXwqV0/f4S88t8mr5hweR5rWqo7M9KlrqpGio7SWv+u/SC84LjkgBwLknOsd93l8nDndvUTOvU1FWyF+bJVm6RnZwlKz7pFDhO8TySyfPa10SCNXWdbEUhe2aGTqem6DQe92DFYs46kPQ9k8lxWkUeZY3RcfKjclVmRUDtdWa8BIxKOMp+RmPkR3kw2lvwHFg6FqPsuxSjf3EU9pEUgNSRKLFv0nX6Hw7ckiKwuCfKrCqPUTclUWIlUeq5KwIq3vcFvljDY9TNbkRyCoBSBNTdiMT8Mu/6rpSxBgQ3B8K/jq+NU5udiOwUgM6d1wF1JyIzd2YzMPjjGRDo/IePQbFxODJB20PesluyPTTMDkcm6EMwfFFuTx4IvekPWH5lt+Rt/0DZXT4DMLJ6fEyUVnMAAAAASUVORK5CYII=\" />";
					divcell.appendChild(a);
				divrow.appendChild(divcell)
			container.appendChild(divrow);
		}		
	}
	else {
		alert(str);
	}

}

function showVal(m) {
  var xhttp = new XMLHttpRequest;
  xhttp.open("POST", "/storm", true);
  xhttp.send("m=" + m);
  window.location = window.location;
}

function zeros(id, num, i) {
	$selection = document.getElementById(id);
	var s = i + num;
	if(i == '00') {$selection.value = s.substr(s.length-3);}
	else if(i == '0') {$selection.value = s.substr(s.length-2);}
}

function hide(id) {
	
  var div = document.getElementById(id);
  //if (div.style.display == 'block')
    div.style.display = 'none';
}

function show(id) {
  var div = document.getElementById(id);
  //if (div.style.display == 'none')
    div.style.display = 'block';
}
function input_fields(value) {
	if (value == "save")	{ show('chip_div'); show('ap_div'); hide('wait_div'); hide('time_div') }
	if (value == "wait")	{ hide('chip_div'); show('ap_div'); show('wait_div'); hide('time_div') }
	if (value == "checkg")	{ hide('chip_div'); show('ap_div'); hide('wait_div'); hide('time_div') }
	if (value == "eraseg")	{ hide('chip_div'); show('ap_div'); hide('wait_div'); hide('time_div') }
	if ( value == "glock")	{ hide('chip_div'); show('ap_div'); hide('wait_div'); hide('time_div') }
	if ( value == "gunlock"){ hide('chip_div'); show('ap_div'); hide('wait_div'); hide('time_div') }
	if ( value == "time")	{ hide('chip_div'); hide('ap_div'); hide('wait_div'); show('time_div') }
}
function replace() {
	if(document.getElementById('chip1').type == 'number') 
	{
		document.getElementById('chip1').type = 'text';
	}
	
	if(document.getElementById('chip1').type == 'text') 
	{
		var str = document.getElementById('chip1').value;
        var str = str.replace(/ |,/g,'*');
		document.getElementById('chip1').value = str;
	}	
}

function addFields(){
	if(document.querySelectorAll('input[name^="chip"]').length == 1) {
		var container = document.getElementById("chip_div");
		for (i=2;i<5;i++){
			// Append a line break 
			container.appendChild(document.createElement("br"));
			
			var input = document.createElement("input");
			input.type = "number";
			input.name = "chip" + i;
			input.className = "semi-square";
			input.autocomplete = "off";
			input.min = "100000000";
			input.setAttribute("max", "10999999999999" );
			container.appendChild(input);
		}
	}
}