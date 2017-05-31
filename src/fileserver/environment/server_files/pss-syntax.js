function highLightPssCode() {
	var highLight = function (element) {
		var orignal = element.innerText;
		var transformed = "";
		const soth = 0;
		const sstr = 1;
		const sid  = 2;
		const scm  = 3;
		var state = soth;
		for(var i = 0; i < orignal.length; i ++)
		{
			var ch = orignal[i];
			if(state == soth)
			{
				if((ch >= 'a' && ch <= 'z') ||
				   (ch >= 'A' && ch <= 'Z') ||
					ch == '_')
				{
					transformed += "[[[[[span class=\"pss-variable\"]]]]]" + ch
					state = sid;
				}
				else if(ch == '"')
				{
					transformed += "[[[[[span class=\"pss-string\"]]]]]\""
					state = sstr;
				}
				else if(ch == '/')
				{
					transformed += "[[[[[span class=\"pss-comment\"]]]]]\/";
					state = scm;
				}
				else transformed += ch;
			}
			else if(state == sstr)
			{
				if(ch == '\"') 
				{
					transformed += "\"[[[[[/span]]]]]"
					state = soth;
				}
				else transformed += ch;
			}
			else  if(state == sid)
			{
				if((ch >= 'a' && ch <= 'z') ||
				   (ch >= 'A' && ch <= 'Z') ||
					ch == '_')
				{
					transformed += ch;
				}
				else 
				{
					transformed += "[[[[[/span]]]]]"
					state = soth;
					i --;
				}
			}
			else if(state = scm)
			{
				if(ch == '\n') 
				{
					transformed += "[[[[[/span]]]]]\n"
					state = soth;
				}
				else transformed += ch;
			}
		}

		return transformed.replace(/>/g, "&gt;").replace(/\[\[\[\[\[/g, "<").replace(/\]\]\]\]\]/g, ">");
	};
	var codes =  document.getElementsByClassName("pss");
	for(var i = 0; i < codes.length; i ++)
		codes[i].innerHTML = highLight(codes[i]);
}
