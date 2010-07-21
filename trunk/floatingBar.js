  floatingBarMenus = [
    {menuID: '001', menuName: 'original', imageURL: 'images/floatingBar_orl.png', status: true},
    {menuID: '002', menuName: 'zoom', imageURL: 'images/floatingBar_zoom.png', status: true},
    {menuID: '003', menuName: 'background', imageURL: 'images/floatingBar_bg.png', status: true},
    {menuID: '004', menuName: 'search', imageURL: 'images/floatingBar_search.png', status: true},
    {menuID: '005', menuName: 'video', imageURL: 'images/floatingBar_video.png', status: true}
  ]

chrome.extension.onRequest.addListener(function(request, sender, response) {
  if (request && request.msg == 'restoreVideoAlone') {
    response(floatingBar.restoreVideoWindow());
  }
});


var floatingBar = {
  listeningElements: ['IMG', 'OBJECT', 'EMBED', 'VIDEO'],
  curElement: null,
  position: null,
  curPicture: null,
  floatingBarStatus: false,
  nodeStyles: [],
  curVideoSize: {videoElement: null, height: null, width: null},
  videoAloneFlag: false,
  

  floatingBarClass: {
    IMG : [
        {menu: floatingBarMenus[0], operate: 'showOriginalPicture', specialCondition: 'checkCurPictureSize'},
        {menu: floatingBarMenus[1], operate: 'magnifier'},
        {menu: floatingBarMenus[2], operate: 'setAsDesktopBackground'},
        {menu: floatingBarMenus[3], operate: 'searchSimilarPicture'},
    ],
    OBJECT : [{menu: floatingBarMenus[4], operate: 'popupVideo'}],
    EMBED : [{menu: floatingBarMenus[4], operate: 'popupVideo'}],
    VIDEO : [{menu: floatingBarMenus[4], operate: 'popupVideo'}]
  },

  onMouseOut: function(listener, id) {
    var curElement = event.target;
    window.setTimeout(function() {
      if (document.getElementById(id) && curElement.parentNode.id != id) {
        document.body.removeChild(document.getElementById(id));
        document.removeEventListener(listener, true);
      }
    }, 1000);
  },

  onMouseMove: function(event) {
    var curElement = event.target;
    var curElementName = curElement.tagName;
    var checkedElements = floatingBar.checkCurrentElement(floatingBar.floatingBarClass, curElementName);
    var hideTime;

    if (checkedElements && !document.getElementById('media_floatingBar')) {
      //floatingBar.curElement = event.target;
      var divElement = document.createElement('div');
      divElement.id = 'media_floatingBar';
      divElement.style.position = 'absolute';
      var position = floatingBar.getCurElementPositionAndSize(curElement);
      divElement.style.left = position.left + 'px';
      divElement.style.top = position.top - 25 + 'px';

      for (var i = 0; i < checkedElements.length; i++) {

        if (checkedElements[i].menu.status) {
          var specialConditionReturnValue = floatingBar.specialCondition(checkedElements[i].specialCondition, curElement)
          if (!checkedElements[i].specialCondition ||
              (checkedElements[i].specialCondition &&
              specialConditionReturnValue)) {
            var imgElement = floatingBar.createImageMenu(checkedElements[i].menu.menuID,
                checkedElements[i].menu.menuName, checkedElements[i].menu.imageURL);
            (function(operates) {
              imgElement.onclick = function() {
                floatingBar.operate(operates, curElement, position);
              }})(checkedElements[i].operate);
              divElement.appendChild(imgElement);
          }

        }
      }
      if (!document.getElementById(divElement.id)) {
        document.body.appendChild(divElement);
      }

      curElement.addEventListener('mouseout', function() {
        floatingBar.onMouseOut(curElement, divElement.id);
      }, false);

    }
    
  },

  checkCurrentElement: function(listeningElements, curElement) {
    //var curElementName = this.curElement.tagName;
    return listeningElements[curElement];
  },

  checkMenuStatus: function() {

  },

  operate: function(todo, curElement, position) {
    switch(todo) {
      case 'showOriginalPicture': floatingBar.showOriginalPicture(curElement, position);
        break;
      case 'magnifier': magnifier.openMagnifier(curElement, position);
        break;
      case 'setAsDesktopBackground':
        break;
      case 'searchSimilarPicture':
        break;
      case 'popupVideo': floatingBar.popupVideoWindow(curElement, position);
        break;

    }
  },

  getCurElementPositionAndSize: function(curElement) {
    var curElementPositionAndSize = {top: 0, left: 0, width: 0, height: 0};
    var node = curElement;
    curElementPositionAndSize.width = node.clientWidth;
    curElementPositionAndSize.height = node.clientHeight;
    while(node && node != document.body) {
      curElementPositionAndSize.top += node.offsetTop;
      curElementPositionAndSize.left += node.offsetLeft;
      node = node.offsetParent;
    }
    return curElementPositionAndSize;
  },

  createImageMenu: function(menuId, menuName, imageURL) {
    var divElement = document.createElement('a');
    divElement.id = menuId;
    divElement.title = menuName;
    divElement.style.backgroundImage ='url(' + chrome.extension.getURL(imageURL) + ')';
    return divElement;
  },

  checkCurrentPictureSize: function(curPicture) {
    var curPicture = curPicture;
    if (curPicture) {
      var orlImage = new Image();
      var orlHeight;
      var orlWidth;
      orlImage.src = curPicture.src;
      orlHeight = orlImage.height;
      orlWidth = orlImage.width;
      if (orlHeight == curPicture.clientHeight && orlWidth == curPicture.clientWidth) {
        return false;
      }
      return true;
    }
  },

  specialCondition: function(condition, curElement) {
    var returnValue;
    switch(condition) {
      case 'checkCurPictureSize':
        returnValue = floatingBar.checkCurrentPictureSize(curElement);
        break;
    }
    return returnValue;
  },

  showOriginalPicture: function(curElement, position) {
    if (document.getElementById('media_originaPicture')) {
      document.body.removeChild(document.getElementById('media_originaPicture'));
    }
    var imgElement = document.createElement('img');
    imgElement.id = 'media_originaPicture';
    imgElement.src = curElement.src;
    imgElement.style.position = 'absolute';
    imgElement.style.left = position.left + 'px';
    imgElement.style.top = position.top + 'px';
    imgElement.style.zIndex = 9999;
    document.body.appendChild(imgElement);
  },

  popupVideoWindow: function(curElement, position) {
    var curElement = curElement;
    var styles = [];
    var tabTitle;
    styles.push({node: document.body, style: document.body.style.cssText});
    styles.push({node: curElement.parentNode, style: curElement.parentNode.style.cssText});

    curElement.parentNode.style.position = 'fixed';
    curElement.parentNode.style.top = 0;
    curElement.parentNode.style.left = 0;
    curElement.parentNode.style.height = '100%';
    curElement.parentNode.style.width = '100%';
    curElement.parentNode.style.zIndex = 9999;
    curElement.parentNode.style.backgroundColor = '#000000';
    floatingBar.curVideoSize = {videoElement: curElement, height: curElement.height, width: curElement.width};
    var nodes = curElement.parentNode.childNodes;

    for (var i = 0; i < nodes.length; i++) {
      try {
        if (nodes[i] != curElement) {
          nodes[i].style.display = 'none';
        }
        styles.push({node: nodes[i], nodeStyle: nodes[i].style.cssText});
      }catch (error) {
        console.log(error);
      }
    }
    floatingBar.nodeStyles = styles;
    console.log(styles);
    document.body.height = position.width;
    document.body.width = position.height;
    document.body.style.overflowX = 'hidden';
    document.body.style.overflowY = 'hidden';
    tabTitle = document.title;
    document.title += Math.random() * 1000000;
 //   window.scrollTo(position.left, position.top);
    floatingBar.videoAloneFlag = true;
    window.onresize = function() {
      if (floatingBar.videoAloneFlag) {
        var visibleHeight = window.innerHeight;
        var visibleWidth = window.innerWidth;
        curElement.style.width = visibleWidth + 'px';
        curElement.style.height = visibleHeight + 'px';
      }
    }

    chrome.extension.sendRequest(
        {msg: 'popupVideoWindow',
         width:position.width,
         height: position.height,
         orgTitle: tabTitle,
         uniqueTitle: document.title
        });
  },

  restoreVideoWindow: function() {
    var nodeStyles = floatingBar.nodeStyles;
    var curVideo = floatingBar.curVideoSize.videoElement;
    curVideo.height = floatingBar.curVideoSize.height;
    curVideo.width = floatingBar.curVideoSize.width
    for (var i = 0; i < nodeStyles.length; i++) {
      nodeStyles[i].node.style.cssText = nodeStyles[i].style;
    }
    floatingBar.videoAloneFlag = false;
    return {msg: 'restoreVideoWindow'};
  }
};
document.addEventListener('mousemove', floatingBar.onMouseMove, false);



var magnifier = {
  zoom: 2,
  canvas: document.createElement('canvas'),

  mouseDown: function(magnifierMask) {
    document.body.removeChild(magnifierMask);
  },

  mouseUp: function() {

  },

  mouseMove: function(curElement, position) {
    //if (!flag) return;
    magnifier.setMagnifierPosition(curElement, position);

  },

  setMagnifierPosition: function(curElement, position) {
    if (!curElement) {
      return;
    }
    var position = position;
    var canvas = this.canvas;
    var image = curElement;

    var img = new Image();
    img.src = curElement.src;
    var positionX = event.pageX - position.left;
    var positionY = event.pageY - position.top;
    var centerX = canvas.width/2;
    var centerY = canvas.height/2;
    var ctx = canvas.getContext('2d');
    var canvasPosX = positionX - centerX;
    var canvasPosY = positionY - centerY;
    var zoomX = image.width * this.zoom / img.width;
    var zoomY = image.height * this.zoom / img.height;
    if (canvasPosX < -centerX)
      canvasPosX = -centerX;
    if (canvasPosY < -centerY)
      canvasPosY = -centerY;
    if (canvasPosX > position.width - centerX)
      canvasPosX =  position.width - centerX;
    if (canvasPosY > position.height - centerY)
      canvasPosY =  position.height - centerY;
    canvas.style.left =  canvasPosX + 'px';
    canvas.style.top =  canvasPosY + 'px';
    canvas.addEventListener('mousemove','',true);

    ctx.globalCompositeOperation = 'source-over';
    var canvasStartX = positionX;
    var canvasStartY = positionY;

    ctx.fillRect(-1, -1, canvas.width+1, canvas.height+1);
    ctx.globalCompositeOperation = 'xor';
    ctx.beginPath();
    ctx.arc(centerX, centerY, centerX, 0, Math.PI*2, true);
    ctx.closePath();
    ctx.fill();

    canvasStartX = (canvasStartX * this.zoom  - centerX) / zoomX ;
    canvasStartY = (canvasStartY * this.zoom  - centerY) / zoomY ;


    if(canvasStartX < 0) {
      canvasStartX = 0;
    } else if(canvasStartX > (image.width*this.zoom - centerX) / zoomX -centerX / this.zoom) {
      canvasStartX = (image.width*this.zoom - centerX) / zoomX - centerX / this.zoom;
    }

    if(canvasStartY < 0) {
      canvasStartY = 0;
    } else if(canvasStartY > (image.height * this.zoom - centerY) / zoomY - centerY / this.zoom) {
      canvasStartY = (image.height*this.zoom - centerY) / zoomY - centerY / this.zoom;
    }

    try {
      ctx.drawImage(image, canvasStartX, canvasStartY, canvas.width/zoomX, canvas.height/zoomY, 0, 0, canvas.width , canvas.height);
    } catch (error) {
      console.log('positionX:' + positionX + ' - canvasStartX:' + canvasStartX);
      console.log('error:' + error);
    }


  },

  openMagnifier: function(curElement, position) {
    var positionAndSize = position;
    var magnifierMask = document.createElement('div');
    var canvas = this.canvas;
    canvas.width = 150;
    canvas.height = 150;
    canvas.id = 'media_magnifier';
    magnifierMask.id = 'media_magnifier_mask';
    magnifierMask.style.width = positionAndSize.width + 'px';
    magnifierMask.style.height = positionAndSize.height + 'px';
    magnifierMask.style.top = positionAndSize.top + 'px';
    magnifierMask.style.left = positionAndSize.left + 'px';
    if (document.getElementById(magnifierMask.id)) {
      document.body.removeChild(document.getElementById(magnifierMask.id));
    }
    magnifierMask.appendChild(canvas);
    document.body.appendChild(magnifierMask);
    magnifierMask.addEventListener('mousemove', function() {
      magnifier.mouseMove(curElement, position);
    }, false);
    magnifierMask.addEventListener('mousedown', function() {
      magnifier.mouseDown(magnifierMask);
    }, false);
    magnifier.setMagnifierPosition(curElement, position);
  }
};

