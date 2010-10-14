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
      + '-' + date.getDate() + ' ' + date.getHours() + ':' +
      (date.getMinutes() < 9 ? ('0' + date.getMinutes()) : date.getMinutes());
}

function isFunction(handler) {
    return Object.prototype.toString.call(handler) === "[object Function]";
}
