drop procedure if exists sp_wiki_login;

DELIMITER //
CREATE PROCEDURE sp_wiki_login
(
   IN  username     varchar(50),
   OUT email        varchar(50),
   OUT name         varchar(50),
   OUT ret          int
)
SQL SECURITY DEFINER
BEGIN
  declare ck_exists int;

  main: begin

    set ret = 0;
  
    -- See if the username is known & member has permission to logon
    select count(*) 
    into ck_exists
    from members m
    where upper(m.username) = upper(username);
-- Commented out: everyone has wiki access.
--    and (fn_check_permission(m.member_id, 'WIKI_ACCESS')=1);

    if (ck_exists = 1) then
      select m.email, concat_ws(' ', ifnull(firstname, ''), ifnull(surname, '')) as name
      into email, name
      from members m 
      where upper(m.username) = upper(username);

      set ret = 1;
      leave main;
    else
      set ret = 0;
      leave main;
    end if;

  end main;
  
END //
DELIMITER ;
