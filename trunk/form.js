function formDataCollection() {
  var forms = document.forms;
  var formInfo = [];
  for (var i = 0; i < forms.length; i++) {
    var inputs = forms[i].getElementsByTagName('input');
    for (var j = 0; j < inputs.length; j++) {
      var type = inputs[j].type;
      if (type == 'text' || type == 'checkbox' || type == 'radio') {
        var itemInfo = {id: '', name: '', value: '', type: ''};
        itemInfo.type = type;
        if (type == 'checkbox' || type == 'radio') {
          itemInfo.value = inputs[j].checked;
        } else {
          if (!!inputs[j].value) {
            itemInfo.value = inputs[j].value;
          } else {
            continue;
          }
        }
        itemInfo.id = inputs[j].id || '';
        itemInfo.name = inputs[j].name || '';
        formInfo.push(itemInfo);
      }

    }
    var textareas = forms[i].getElementsByTagName('textarea');
    for (var j = 0; j < textareas.length; j++) {
      var itemInfo = {id: '', name: '', value: '', type: 'textarea'};
      if (textareas[j].value != '' && !textareas.getAttribute('readonly')) {
        itemInfo.value = textareas[j].innerHTML;
        itemInfo.id = textareas[j].id || '';
        itemInfo.name = textareas[j].name || '';
        formInfo.push(itemInfo);
        console.log(itemInfo);
      }
    }

    var selects = forms[i].getElementsByTagName('select')
    for (var j = 0; j < selects.length; j++) {
      var itemInfo = {id: '', name: '', value: '', type: 'select'};
      if (selects[j].value != '') {
        itemInfo.value = selects[j].value;
          itemInfo.id = selects[j].id || '';
          itemInfo.name = selects[j].name || '';
        formInfo.push(itemInfo);
      }
    }
  }
  return formInfo;
}

function sendFormData(url, title) {
  var formInfo = formDataCollection();
  chrome.extension.sendRequest({msg: 'saveForm',
                                      formInfo: formInfo,
                                      url: url,
                                      title: title}, function(response){
    
  });
}

function fillForm(formInfo) {
  //var formInfoObj = JSON.parse(formInfo);
  for (var i = 0; i < formInfo.length; i++) {
    var type = formInfo[i].type;
    var id = formInfo[i].id;
    var name = formInfo[i].name;
    var value = formInfo[i].value;
    switch(type) {
      case 'text':
        fillText(id, name, value);
        break;
      case 'checkbox':
      case 'radio':
        fillCheckboxAndRadio(id, name, value);
        break;
      case 'select':
        fillSelect(id, name, value);
        break;
      case 'textarea':
        fillTextarea(id, name, value);
    }
  }
}

function fillText(id, name, value) {
  if (id) {
    document.getElementById(id).value = value
  } else if(name) {
    document.getElementsByName(name)[0].value = value
  }
}

function fillCheckboxAndRadio(id, name, value) {
  if (id) {
    document.getElementById(id).checked = value
  } else if(name) {
    document.getElementsByName(name)[0].checked = value
  }
}

function fillSelect(id, name, value) {
  if (id) {
    document.getElementById(id).value = value
  } else if(name) {
    document.getElementsByName(name)[0].value = value
  }
}

function fillTextarea(id, name, value) {
  if (id) {
    document.getElementById(id).innerHTML = value
  } else if(name) {
    document.getElementsByName(name)[0].innerHTML = value
  }
}