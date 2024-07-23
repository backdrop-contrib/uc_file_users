/**
 * @file
 * JS for uc_file_users module. Copy email addresses to the clipboard.
 */

(function ($) {

Backdrop.behaviors.ucFileUsersEmails = {
  attach: function (context) {
    $('#edit-copy-emails', context).on('click', function() {
      var text = $("#uc-file-users-emails").html();
      // Use a <textarea> so that newlines will be preserved when copying.
      var $temp = $('<textarea id="uc-file-users-emails-textarea">');
      $("body").append($temp);
      $temp.val(text).select();
      document.execCommand("copy");
      $temp.remove();
    });
  }
};

})(jQuery);
