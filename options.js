function $(id) {
  var element = document.getElementById(id);
  if (element) {
    return element;
  } else {
    //throw new Error('$: not found:' + id);
    return false;
  }
}

function i18nReplace(id, messageName) {
  $(id).innerText = chrome.i18n.getMessage(messageName);
}
var fillForm = new FillForm();
var shortcut = new Shortcut();
function setMessage() {
  var i18n_map = [
    {id: 'optionTitle', message: 'option_title'},
    {id: 'mGeneral', message: 'tab_general' },
    {id: 'mFillForm', message: 'tab_fill_form' },
    {id: 'mQuicklyVisit', message: 'tab_quick_launch'},
    {id: 'mShortcut', message: 'tab_shortcut'},
    {id: 'mRecommended', message: 'tab_recommended'},
    {id: 'title_floatingBar', message: 'title_floating_bar'},
    {id: 'title_tab', message: 'title_tab'},
    {id: 'item_imageBar', message: 'item_image_bar'},
    {id: 'item_videoBar', message: 'item_video_bar'},
    {id: 'item_closeLastTab', message: 'item_close_last_tab'},
    {id: 'item_openInNewTab', message: 'item_open_in_new_tab'},
    {id: 'item_dbClickCloseTab', message: 'item_double_click_close_tab'},
    {id: 'fillForm_title', message: 'fill_form_title'},
    {id: 'fillForm_address', message: 'fill_form_url'},
    {id: 'fillForm_date', message: 'fill_form_date'},
    {id: 'fillFrom_delAll', message: 'fill_from_delete_all'},
    {id: 'shortcut_compare', message: 'shortcut_compare'},
    {id: 'quicklyVisit_selectedFolder', message: 'quick_launch_select_folder'},
    {id: 'quicklyVisit_manageBookmark', message: 'quick_launch_manage_bookmark'},
    {id: 'quicklyVisit_close', message: 'quick_launch_close'},
    {id: 'bookmark_tip', message: 'bookmark_tip'},
    {id: 'gotoShortcutTab', message: 'bookmark_tip2'}
  ];
  document.title = chrome.i18n.getMessage('option_title');
  for (var i = 0; i < i18n_map.length; i++) {
    var id = i18n_map[i].id;
    var name = i18n_map[i].message;
    i18nReplace(id, name);
  }
}

function Option() {
  var self = this;

  //checkbox element
  this.imageBar = $('imageBar');
  this.videoBar = $('videoBar');
  this.closeLastTab = $('closeLastTab');
  this.openInNewTab = $('openInNewTab');
  this.dbclickCloseTab = $('dbclickCloseTab');
  this.isCompare = $('isCompare');

  //div element
  this.nav_general = $('mGeneral');
  this.nav_fillForm = $('mFillForm');
  this.nav_quicklyVisit = $('mQuicklyVisit');
  this.nav_shortcut = $('mShortcut');
  this.nav_recommended = $('mRecommended');

  this.tab_general = $('generalTab');
  this.tab_fillForm = $('fillFormTab');
  this.tab_quicklyVisit = $('quicklyVisitTab');
  this.tab_shortcut = $('shortcutTab');
  this.tab_recommended = $('recommendedTab');

  //form tab element
  this.form_deleteAll = $('form_deleteAll');
  this.form_close = $('form_close');
  this.form_content = $('formContent');

  //a element
  this.gotoShortcutTab = $('gotoShortcutTab');


  this.table_shortcut = $('shortcutTable');

  this.setNavigationBarStatus(this.nav_general);
  this.nav_general.addEventListener('click', this.setNavigationBarStatus, false);
  this.nav_fillForm.addEventListener('click',
      this.setNavigationBarStatus, false);
  this.nav_quicklyVisit.addEventListener('click',
      this.setNavigationBarStatus, false);
  this.nav_shortcut.addEventListener('click',
      this.setNavigationBarStatus, false);
  this.nav_recommended.addEventListener('click',
      this.setNavigationBarStatus, false);

  createQuickLaunchTab();
  
  fillForm.showAllFormDate();
  this.form_deleteAll.addEventListener('click', function() {
    fillForm.deleteAllData();
    fillForm.showAllFormDate();
  }, false);
  this.form_close.addEventListener('click', function() {
    this.form_content.style.display = 'none';
  }, false);


  var categorySelect = shortcut.createSelect(key_util.category_table);
  var browserSelect = shortcut.createSelect(key_util.browser);

  this.isCompare.addEventListener('change', function() {
    self.table_shortcut.innerHTML = '';
    self.table_shortcut.appendChild(
      shortcut.showTable(categorySelect, browserSelect, self.isCompare.checked));
  }, false);
  categorySelect.addEventListener('change', function() {
    self.table_shortcut.innerHTML = '';
    self.table_shortcut.appendChild(
      shortcut.showTable(categorySelect, browserSelect, self.isCompare.checked));
  }, false);
  browserSelect.addEventListener('change', function() {
    self.table_shortcut.innerHTML = '';
    self.table_shortcut.appendChild(
      shortcut.showTable(categorySelect, browserSelect, self.isCompare.checked));
  }, false);
  this.table_shortcut.appendChild(
      shortcut.showTable(categorySelect, browserSelect, self.isCompare.checked));
  this.setGeneralTabOption();

  this.gotoShortcutTab.addEventListener('click', function() {
    categorySelect.value = key_util.category_table.CAT_QUICK_LAUNCH;
    self.table_shortcut.innerHTML = '';
    self.table_shortcut.appendChild(
      shortcut.showTable(categorySelect, browserSelect, self.isCompare.checked));
    self.setNavigationBarStatus(self.nav_shortcut);
  }, false)
  setMessage();
}

Option.prototype.setNavigationBarStatus = function(element) {
  var this_obj = this;
  if (arguments.length == 1 && arguments[0] != event) {
    this_obj = element;
  }
  var navigationBarMap = [
      {menu: 'mGeneral', tab: 'generalTab'},
      {menu: 'mFillForm', tab: 'fillFormTab'},
      {menu: 'mQuicklyVisit', tab: 'quicklyVisitTab'},
      {menu: 'mShortcut', tab: 'shortcutTab'},
      {menu: 'mRecommended', tab: 'recommendedTab'}];
  for (var i = 0; i < navigationBarMap.length; i++) {
    var navMenu = $(navigationBarMap[i].menu);
    var tab = $(navigationBarMap[i].tab);
    if (navMenu == this_obj) {
      navMenu.className = 'choosed';
      tab.style.display = 'block';
    } else {
      navMenu.className = '';
      tab.style.display = 'none';
    }
  }
};

Option.prototype.setGeneralTabOption = function() {
  this.imageBar.checked = eval(localStorage['imageBar']);
  this.videoBar.checked = eval(localStorage['videoBar']);
  this.closeLastTab.checked = eval(localStorage['closeLastTab']);
  this.openInNewTab.checked = eval(localStorage['openInNewTab']);
  this.dbclickCloseTab.checked = eval(localStorage['dbclickCloseTab']);
  this.imageBar.addEventListener('change', function() {
    localStorage['imageBar'] = $('imageBar').checked;
    showSavingSucceedTip();
  });
  this.videoBar.addEventListener('change', function() {
    localStorage['videoBar'] = $('videoBar').checked;
    showSavingSucceedTip();
  });
  this.closeLastTab.addEventListener('change', function() {
    localStorage['closeLastTab'] = $('closeLastTab').checked;
    showSavingSucceedTip();
  });
  this.openInNewTab.addEventListener('change', function() {
    localStorage['openInNewTab'] = $('openInNewTab').checked;
    showSavingSucceedTip();
  });
  this.dbclickCloseTab.addEventListener('change', function() {
    localStorage['dbclickCloseTab'] = $('dbclickCloseTab').checked;
    var bg = chrome.extension.getBackgroundPage();
    bg.dbClickCloseTab();
    showSavingSucceedTip();
  });
}

function showSavingSucceedTip() {
  var div = document.createElement('DIV');
  div.className = 'tip_succeed';
  div.innerText = chrome.i18n.getMessage('tip_succeed');
  document.body.appendChild(div);
  div.style.left = (document.body.clientWidth - div.clientWidth) / 2 + 'px';
  window.setTimeout(function() {
    document.body.removeChild(div);
  }, 2000);
}

function showSavingFailedTip(msg) {
  var div = document.createElement('DIV');
  div.className = 'tip_failed';
  div.innerText = chrome.i18n.getMessage(msg);
  document.body.appendChild(div);
  div.style.left = (document.body.clientWidth - div.clientWidth) / 2 + 'px';
  window.setTimeout(function() {
    document.body.removeChild(div);
  }, 5000);
}

function init() {
  new Option();
}


