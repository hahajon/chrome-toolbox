var database = openDatabase("convenience_database", "1.0", "convenience", 5*1024*1024);

var db = {
  executeSqlCommand: function(sql, args, callBack) {
    if (database) {
      database.transaction(function(tx) {
        tx.executeSql(sql, args, callBack, db.logger);
      });
    } else {
      console.log('fail');
    }
  },

  logger: function(tx, error) {
    console.log(error.message);
  }
};

var FillForm = function() {
  this.createFillFormTable = function() {
    var sql = 'create table if not exists fillForm_table(' +
              'id integer primary key,' +
              'url varchar,' +
              'title varchar,' +
              'formInfo text,' +
              'date datetime)';
    db.executeSqlCommand(sql, null, null);
  };

  this.showContentById = function(id) {
    var sql = 'select * from fillForm_table where id = ?';
    db.executeSqlCommand(sql, [id], function(tx, results) {
      if (results.rows.length > 0) {
        var title = document.getElementById('divTitle');
        var url = document.getElementById('divURL');
        title.innerText = 'Title:'+results.rows.item(0).title;
        url.innerText = 'URL:' + results.rows.item(0).url;
        var formInfo = JSON.parse(results.rows.item(0).formInfo);
        var str = '';
        str = '<table id="tt" cellpadding="0" cellspacing="0" border="0">'
            + '<thead>'
            + '<tr>'
            + '<td style="width:150px;">字段</td>'
            + '<td style="width:250px;">内容</td>'
            + '<td style="width:100px;">类型</td>'
            + '</tr>'
            + '</thead>';
        for (var i = 0; i < formInfo.length; i++) {
          str += '<tr>'
              + '<td>' + (formInfo[i].id || formInfo[i].name) + '</td>'
              + '<td>' + formInfo[i].value + '</td>'
              + '<td>' + formInfo[i].type + '</td>'
              + '</tr>';
        }
        str += '</table>';
        document.getElementById('divFromInfoTable').innerHTML = str;
        document.getElementById('formContent').style.left = window.innerWidth/2 - 250 + 'px';
        document.getElementById('formContent').style.display = 'block';
      }
    });
  };

  this.showAllContent = function(){
    var sql = 'select * from fillForm_table';
    var str = '';
    db.executeSqlCommand(sql, null, function(tx, results) {
      str = '<table border="0" cellspacing="0" cellpadding="0">';
      for (var i = 0; i < results.rows.length; i++) {
        var id = results.rows.item(i).id;
        var title = results.rows.item(i).title;
        var url = results.rows.item(i).url;
        var date = results.rows.item(i).date;
        str += '<tr>';
        str += '<td><div class="ff_title">' + title + '</div></td>';
        str += '<td><div class="ff_url">' + url + '</div></td>';
        str += '<td><div class="ff_date">' + date + '</div></td>';
        str += '<td><div class="ff_operate"><a href="#" onclick="fillForm.showContentById(' +
            id + ')">View</a> <a href="#" onclick="fillForm.deleteById('+
            id + ');">Del</a></div>';
        str += '</tr>';
      }
      str += '</table>';
      document.getElementById('allFormInfo').innerHTML = str;
    });
  };



  this.getKeyboardPressKey = function() {
    document.body.onkeydown = function() {
      var keyCode = [];
      if (event.ctrlKey) {
        keyCode.push(event.ctrlKey);
      }
      if (event.shiftKey) {
        keyCode.push(event.shiftKey);
      }
      if (event.altKey) {
        keyCode.push(event.altKey);
      }
      if (!event.ctrlKey && !event.altKey && !event.shiftKey) {
        keyCode.push(event.keyCode);
      }
      return keyCode.join(' + ');
    }
  }

  this.deleteAllData = function() {
    var sql = 'delete from fillForm_table';
    db.executeSqlCommand(sql, null, null);
  };

  this.deleteById = function(id) {
    var sql = 'delete from fillForm_table where id = ?';
    db.executeSqlCommand(sql, [id], null);
  };

  this.deleteByUrl = function(url) {
    var sql = 'delete from fillForm_table where url = ?';
    db.executeSqlCommand(sql, [url], null);
  };

  this.update = function(formInfo, title, id) {
    var sql = 'update fillForm_table set formInfo = ?, title = ?, date = ? where id = ?';
    db.executeSqlCommand(sql, [formInfo, title, getDate(), id], null);
  };

  this.selectByUrl = function(url, callBack) {
    var sql = 'select * from fillForm_table where url = ?';
    db.executeSqlCommand(sql, [url], callBack);
  };

  this.save = function(url, title, formInfo) {
    var sql = 'insert into fillForm_table(url, title, formInfo, date)values(?, ?, ? ,?)';
    db.executeSqlCommand(sql, [url, title, formInfo, getDate()], null);
  };
};

var Shortcut = function() {
  this.createShortcutTable = function() {
    var sql = 'create table if not exists shortcut_table(' +
              'id integer primary key,' +
              'name varchar,' +
              'shortcut varchar,' +
              'status bool,' +
              'type bool,' +
              'operation varchar,' +
              'modifiable bool,' +
              'extensionId varchar,' +
              'date datetime)';
    db.executeSqlCommand(sql, null, null);
  };

  this.insert = function(obj) {
    var sql = 'insert into shortcut_table(id, name, ' +
        'shortcut, status, type, operation, modifiable, extensionId, date)' +
        'values(?, ?, ?, ?, ?, ?, ?, ?, ?)';
    db.executeSqlCommand(sql, [obj.id, obj.name, obj.shortcut, obj.status,
        obj.type, obj.operation, obj.modifiable, obj.extensionId, getDate()], null);
  };

  this.selectById = function(id, resultsCallback) {
    var sql = 'select * from shortcut_table where id = ?';
    db.executeSqlCommand(sql, [id], resultsCallback);
  };

  this.showAllShortcut = function(id) {
    var sql = 'select * from shortcut_table';
    db.executeSqlCommand(sql, null, function(tx, results) {
      var str = '<table border="0" cellspacing="0" cellpadding="0">';
      if (results.rows.length > 0) {
        for (var i = 0; i < results.rows.length; i++) {
          var pid = results.rows.item(i).id;
          var name = results.rows.item(i).name;
          var shortcut = results.rows.item(i).shortcut;
          str += '<tr>' +
                 '<td class="name">' + name + '</td>' +
                 '<td class="key"><div id="' + pid +
                 '"value="' + shortcut + '" onclick="shortcut.addEvent(this)">' +
                 keyCodeToShowName(shortcut) + '</div></td>' +
                 '</tr>';
        }
        str += '</table>';
        document.getElementById(id).innerHTML = str;
      }
    });
  };

  this.addEvent = function(element) {
      element.innerText = '输入按键';
      document.body.onkeydown = function() {
        var keyCode = [];
        if (event.ctrlKey) {
          keyCode.push(17);
        }
        if (event.shiftKey) {
          keyCode.push(16);
        }
        if (event.altKey) {
          keyCode.push(18);
        }
        if (!event.ctrlKey && !event.altKey && !event.shiftKey) {
          keyCode.push(event.keyCode);
        }
        element.value = keyCode.join(' + ');
        element.innerText = keyCode.join(' + ');
        //element.innerText = keyCodeToShowName(keyCode.join(' + '));
      }
    }
}

function logger(tx, error) {
  console.log(error.message);
}

function getDate() {
  var date = new Date();
  return date.getFullYear() + '-' + (date.getMonth()+1) + '-' + date.getDate();
}
