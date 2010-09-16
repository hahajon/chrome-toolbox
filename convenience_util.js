function $(id) {
  var element = document.getElementById(id);
  if (element) {
    return element;
  } else {
    //throw new Error('$: not found:' + id);
    return false;
  }
}

function getDate() {
  var date = new Date();
  return date.getFullYear() + '-' + (date.getMonth() + 1)
      + '-' + date.getDate();
}

function isFunction(handler) {
    return Object.prototype.toString.call(handler) === "[object Function]";
}
