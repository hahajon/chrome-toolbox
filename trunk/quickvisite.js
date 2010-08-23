var bookmarkParentNodeId = 0;
  var str = '';
  function openBookmark(hotKey) {
    chrome.bookmarks.getTree(function(bookmarkTreeNode) {
      console.log(bookmarkTreeNode);
      document.getElementById('openHitKey').value = hotKey;
      str = '<ul>';
      createBookmarkTreeNode(bookmarkTreeNode);
      document.getElementById('bookmarkBox').style.display = 'block';
    });
  }
  function createBookmarkTreeNode(bookmarkTreeNode) {

    for (var i = 0; i < bookmarkTreeNode.length; i++) {
      str += '<li class="close" id="' + bookmarkTreeNode[i].id + '">'
      var node = bookmarkTreeNode[i];
      if (node.children) {
        if (node.title != '') {
          str += '<div name="parent" onclick="openOrCloseBookmarkTree('+ node.id + ')">' + node.title + '</div>';
          str += '<ul>';
        }
        createBookmarkTreeNode(node.children);
      } else {
        str += '<a href="' + node.url + '" ><img src="chrome://favicon/' + node.url + '" width="16" height="16" alt="" />' + node.title + '</a>';
      }
      str += '</li>';
      if (i == bookmarkTreeNode.length-1) str += '</ul></li>';
    }
    document.getElementById('bookmarkTree').innerHTML = str + '<ul>';
  }

  function closeBox(id) {
    document.getElementById(id).style.display = 'none';
  }

  function openBookmarkTab() {
    document.getElementById('bookmarkBox').style.display = 'none';
    chrome.tabs.create({url:'chrome://bookmarks/', selected:true});
  }

  function openOrCloseBookmarkTree(id) {
    var element = document.getElementById(id);
    if (element.className && element.className == "open") {
      element.className = 'close';
    } else if(element.className == "close"){
      element.className = 'open';
    }
    var parent = document.getElementsByName('parent');
    for (var i = 0; i < parent.length; i++) {
      parent[i].className = '';
    }
    event.target.className = 'selected';
    bookmarkParentNodeId = id;
  }
  function getSelectedBookmarkLinks() {
    var links = [];

    var traversalAllNode = function(nodeId) {
      chrome.bookmarks.getChildren(nodeId, function(children) {
        console.log("nodes:" + children);
          for (var i = 0; i < children.length; i++) {
            if (children[i].dateGroupModified) {
              traversalAllNode(children[i].id.toString());
            } else {
              links.push(children[i].url);
              console.log("links:" + children[i].url);
            }
          }
        console.log("links:" + links.length);
      });
    }
    if (bookmarkParentNodeId) {
      chrome.bookmarks.get(bookmarkParentNodeId.toString(), function(bookmarkTreeNode) {
        var hotKey = document.getElementById(document.getElementById('openHitKey').value);
        hotKey.value = bookmarkTreeNode[0].title;
        saveOpenTabsHotKey(hotKey.id, bookmarkTreeNode[0].id)
      });
      traversalAllNode(bookmarkParentNodeId.toString());
      document.getElementById('bookmarkBox').style.display = 'none';
    }
  }

  function getOpenTabsHotKey() {
    var openTabsHotKey = localStorage.openTabsHotKey ? localStorage.openTabsHotKey: '';
    var hotKeys = openTabsHotKey.split(';');
    for (var i = 0; i < hotKeys.length; i++) {
      var hotKey = hotKeys[i].split(':');
      var id = hotKey[0];
      (function(id) {
        chrome.bookmarks.get(hotKey[1], function(bookmarkTreeNode) {
        document.getElementById(id).value = bookmarkTreeNode[0].title;
      });
      })(id)


    }
  }

  function saveOpenTabsHotKey(hotKey, nodeID) {
    var openTabsHotKey = localStorage.openTabsHotKey ? localStorage.openTabsHotKey: '';
    var array = openTabsHotKey.split(';');
    var flag = false;
    for (var i = 0; i < array.length; i++) {
      if (array[i].indexOf(hotKey) == 0) {
        flag = true;
        array[i] = hotKey + ':' + nodeID;
        break;
      }
    }
    if (flag) {
      localStorage.openTabsHotKey = array.join(";");
    } else {
      localStorage.openTabsHotKey += ";" + hotKey + ':' + nodeID;
    }

  }

