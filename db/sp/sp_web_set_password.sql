drop procedure if exists sp_web_set_password;

/*


*/

DELIMITER //
CREATE PROCEDURE sp_web_set_password
(
   IN  p_member_id  int,
   IN  new_salt     varchar(16),
   IN  new_password varchar(40),
   OUT err          varchar(100)
)
SQL SECURITY DEFINER
BEGIN

  declare ck_exists int;
  declare m_member_id int;

  main: begin
    
    set err = '';  
 
    select count(*) 
    into ck_exists
    from members m
    inner join members_auth a on a.member_id = m.member_id
    where m.member_id = p_member_id;

    if (ck_exists = 1) then
      update members_auth
      set 
        salt   = new_salt,
        passwd = new_password
      where member_id = p_member_id;
    else
      insert into members_auth (member_id, salt, passwd)
      values (p_member_id, new_salt, new_password);
    end if;
      
  end main;

END //
DELIMITER ;

