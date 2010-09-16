  var selectedBookmarkNodeId = 0;
  var quickLaunchShortcutId = 0;
  function openMyBookmarkManager(id) {
    createBookmarkFolder();
    quickLaunchShortcutId = id;
  }

  function createBookmarkFolder() {
    chrome.bookmarks.getTree(function(bookmarkTreeNode) {
      $('bookmarkTree').innerHTML = '';
      var ul = document.createElement('UL');
      ul.id = 'bookmark_folder_menu';
      var getBookmarkFolder = function(parent, bookmarkTreeNode) {
        for (var i = 0; i < bookmarkTreeNode.length; i++) {
          var li = document.createElement('li');
          var node = bookmarkTreeNode[i];
          if (node.children) {
            li.id = 'bookmarkNode_' + node.id;
            var url = chrome.extension.getURL('images/folder_close.png');
            li.innerHTML = '<img src ="' + url + '" alt="">' +  node.title;
            (function(id) {
              li.onclick = function() {
                createSelectedBookmarkFolderLinks(id);
                setSelectedFolderClass(id);
              }
            })(node.id);
            parent.appendChild(li);
            getBookmarkFolder(parent, node.children);
          }
        }
      };
      getBookmarkFolder(ul, bookmarkTreeNode);
      $('bookmarkTree').appendChild(ul);
      $('bookmarkBox').style.display = 'block';
    });
  }

  function createQuickLaunchTab() {
    var quickLaunchIds = [51, 52, 53, 54, 55, 56, 57, 58, 59];
    //var shortcut = new Shortcut();
    var table = document.createElement('TABLE');
    for (var i = 0; i < quickLaunchIds.length; i++) {
      shortcut.selectById(quickLaunchIds[i], function(tx, results) {
        if (results.rows.length > 0) {
          var tr = document.createElement('TR');
          var td1 = document.createElement('td');
          td1.innerText = results.rows.item(0).shortcut;
          tr.appendChild(td1);
          var td2 = document.createElement('td');
          var input = document.createElement('INPUT');
          input.id = 'input_' + results.rows.item(0).id;
          input.type = 'text';
          input.size = '52';
          input.readOnly = true;
          td2.appendChild(input);
          tr.appendChild(td2);
          var td3 = document.createElement('td');
          var img = document.createElement("IMG");
          img.src = chrome.extension.getURL('images/folder_open.png');
          (function(id, selectedNodeId) {
            input.onclick = function() {
              if (!input.value) {
                input.value = chrome.i18n.getMessage('bookmark_tip3');
                input.style.color = '#d9d9d9';
              }
            };
            setSelectedBookmarkFolderName(input, selectedNodeId);
            img.onclick = function() {
              openMyBookmarkManager(id);
            };
          })(results.rows.item(0).id, results.rows.item(0).relationId);
          td3.appendChild(img);
          tr.appendChild(td3);
          table.appendChild(tr);
        }
      });
    }
    $('openBookmark').appendChild(table);
  }

  function openBookmarkFolderLinks(nodeId) {
    chrome.bookmarks.getChildren(nodeId, function(children) {
      for (var i = 0; i < children.length; i++) {
        if (children[i].url) {
          chrome.tabs.create({url: children[i].url});
        } else {
          openBookmarkFolderLinks(children[i].id.toString());
        }
      }
    });
  }

  function createSelectedBookmarkFolderLinks(node) {
    $('bookmarkList').innerHTML = '';
    var ul = document.createElement('UL');
    var traversalAllNode = function(nodeId) {

      chrome.bookmarks.getChildren(nodeId, function(children) {
        console.log("nodeid:" + nodeId + 'list:' + children);
        for (var i = 0; i < children.length; i++) {
          if (children[i].url) {
            var li = document.createElement('li');
            li.innerHTML = '<a href="' + children[i].url +
                '" ><img src="chrome://favicon/' + children[i].url +
                '" width="16" height="16" target="_blank" alt=""/>' +
                children[i].title + '</a>';
            ul.appendChild(li);
            console.log("links:" + children[i].url);
          } else {
            traversalAllNode(children[i].id);
          }
        }
      });
    }
    traversalAllNode(node);
    $('bookmarkList').appendChild(ul);
  }

  function closeBox(id) {
    $(id).style.display = 'none';
  }

  function openBookmarkTab() {
    $('bookmarkBox').style.display = 'none';
    chrome.tabs.create({url:'chrome://bookmarks/', selected:true});
  }

  function setSelectedFolderClass(id) {
    if (id) {
      var children = $('bookmark_folder_menu').childNodes;
      for (var i = 0; i < children.length; i++) {
        children[i].className = '';
      }
      $('bookmarkNode_' + id).className = 'selected';
      selectedBookmarkNodeId = id;
    }

  }

  function setSelectedBookmarkFolderName(element, nodeId) {
    if (nodeId) {
      chrome.bookmarks.get(nodeId.toString(), function(bookmarkTreeNode) {
        if (bookmarkTreeNode.length > 0) {
          element.value = bookmarkTreeNode[0].title;
          element.style.color = '#333333';
        }
      });
    }
  }

  function setSelectedBookmarkFolder() {
    if (selectedBookmarkNodeId && quickLaunchShortcutId) {
      console.log(selectedBookmarkNodeId);
      var inputId = 'input_' + quickLaunchShortcutId;
      setSelectedBookmarkFolderName($(inputId) ,selectedBookmarkNodeId);
      var shortcut = new Shortcut();
      shortcut.updateRelationId(selectedBookmarkNodeId, quickLaunchShortcutId);
      selectedBookmarkNodeId = 0;
      quickLaunchShortcutId = 0;
      showSavingSucceedTip();
      $('bookmarkBox').style.display = 'none';
    } else {
      showSavingFailedTip('tip_failed2');
    }
  }



