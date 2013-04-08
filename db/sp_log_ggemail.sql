
drop procedure if exists sp_log_ggemail;

/*
  Log an email received from the google group to gg_email

*/

DELIMITER //
CREATE PROCEDURE sp_log_ggemail
(
  IN ggemail_subj     varchar(200),
  IN ggemail_body     varchar(10000),
  IN ggemail_reply_to varchar(200),
  IN ggemail_msg_id   varchar(200),
  IN ggemail_from     varchar(200),
  OUT err             varchar(100),
  OUT ggemail_id      int
)
SQL SECURITY DEFINER
BEGIN

  main: begin
    
    set err = '';  
 
    insert into gg_email (ggemail_subj, ggemail_body, ggemail_from, ggemail_reply_to, ggemail_msg_id)
    values               (ggemail_subj, ggemail_body, ggemail_from, ggemail_reply_to, ggemail_msg_id);

    set ggemail_id = last_insert_id();
      
  end main;

END //
DELIMITER ;

