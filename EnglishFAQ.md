### Why does this extension require accessing all data on my computer? ###
It's because this extension uses an NPAPI plugin to support features like double click to close a tab. As long as you install an extension using an NPAPI plugin, Chrome will warn you like this.

### Why can't I sync this extension access multiple computers? ###
Based on the security concern, Chrome doesn't sync any extension which contains an NPAPI plugin.

### Why doesn't this extension work some time after I install it? ###
Do you have an anti Trojan software installed? If so, please check your anti Torjan software and see if it records convenience.dll as a Torjan. If so, remove it and select to trust it when your anti Trojan software warn you next time. If you don't see your anti Trojan software warns you, please change its setting to manual, ie. warn you before it blocks Trojans.

### Mute tabs doesn't work in Windows Vista/XP! ###
In Chrome 9.0 and higher versions, Flash process is sandboxed so we can't find a way to mute Flash. It works in Win7 because we use a different implementation.

### Why is the option page always loading? ###
Please check if there is a message box, eg. a message box saying "Boss key is registered by another application, please redefine it". If there isn't, please try restarting your browser.

### Can I hide the browser action icon? ###
In Chrome 10.0 and higher versions, you can right click on the icon and click on "Hide button".

### Standalone video window is awful. Can you make it prettier? ###
Now we create a normal window for standalone video. We hide the address bar but the tab bar is still there. In Chrome 10.0 and higher versions, we will create a popup window for standalone video, so that it will look prettier.

### Why can't I install this extension on Chrome OS? ###
This extension contains an NPAPI plugin, which is unsupported on Chrome OS.