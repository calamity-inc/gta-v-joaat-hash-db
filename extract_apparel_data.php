<?php
// Before running: Search OpenIV for "_shop.meta" and extract all results into the same folder as this script.

function processItems($items)
{
	global $fh;
	if($items->Item)
	{
		foreach($items->Item as $item)
		{
			fwrite($fh, $item->uniqueNameHash."\n");
		}
	}
}

$fh = fopen("raw/apparel_data.txt", "w");
foreach(scandir(".") as $file)
{
	if(substr($file, -5) == ".meta")
	{
		echo $file."\n";
		$data = simplexml_load_file($file);
		processItems($data->pedOutfits);
		processItems($data->pedComponents);
		processItems($data->pedProps);
	}
}
fclose($fh);
