drop procedure if exists sp_web_change_password;

/*


*/

DELIMITER //
CREATE PROCEDURE sp_web_change_password
(
   IN  p_member_id  int,
   IN  old_password varchar(40),
   IN  new_salt     varchar(16),
   IN  new_password varchar(40),
   OUT err          varchar(100)
)
SQL SECURITY DEFINER
BEGIN

  declare ck_exists int;
  declare m_member_id int;

  main: begin
  
    -- Check current password is correct
    select count(*) 
    into ck_exists
    from members m
    inner join members_auth a on a.member_id = m.member_id
    where m.member_id = p_member_id
      and a.passwd = old_password;

    if (ck_exists = 1) then
      update members_auth
      set 
        salt   = new_salt,
        passwd = new_password
      where member_id = p_member_id;
      
      set err = '';
    else
      set err = 'Current password entered incorrect';
    end if;
    
  end main;

END //
DELIMITER ;
