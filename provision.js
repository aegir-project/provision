Drupal.provisionHelpAttach = function() {
  $('.provision-help-toggle').click(
    function() {
      $('.provision-help', $(this).parent()).toggle('slow')
    }
  );
}
if (Drupal.jsEnabled) {
  $(document).ready(Drupal.provisionHelpAttach);
}


