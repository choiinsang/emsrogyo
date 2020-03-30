
var a = function()
{
	var b;
	print("LLL");
}

function xyz(arg1, arg2)
{
	print(arg1);
	return;

	for(var i=0; i<64; i++)
	{
		print(i, arg2[i]);
	}
}

var sum = 0;
for(var i=0; i<100; i++)
{
	sum += i;
}

print("The sum from 0 to 100 is " + sum);
eval("a");
eval("a()");
print(eval("print"));
try
{
	eval("b");
}
catch(e)
{
	print("Error : " + e);
}

var o = new JSTest();
print(o.add(5, 6, 7));

print(o.add);
