
drop procedure if exists sp_log_email;

/*


*/

DELIMITER //
CREATE PROCEDURE sp_log_email
(
  IN member_id       int,
  IN email_to        varchar(200),
  IN email_cc        varchar(200),
  IN email_bcc       varchar(200),
  IN email_subj      varchar(200),
  IN email_body      text,
  IN email_body_alt  text,
  OUT err            varchar(100),
  OUT email_id       int
)
SQL SECURITY DEFINER
BEGIN

  declare ck_exists int;
  declare m_member_id int;

  main: begin
    
    set err = '';  
 
    insert into emails (member_id, email_to, email_cc, email_bcc, email_subj, email_body, email_body_alt, email_status, email_date)
    values (member_id, email_to, email_cc, email_bcc, email_subj, email_body, email_body_alt, 'PENDING', now());

    set email_id = last_insert_id();
      
  end main;

END //
DELIMITER ;
