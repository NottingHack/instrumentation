drop procedure if exists sp_add_member;
/* err is null on success, and member_id is the newly added members id */
DELIMITER //
CREATE PROCEDURE sp_add_member
(
   IN member_number   int,
   IN name            varchar(50),
   IN handle          varchar(100),
   IN unlock_text     varchar(95),
   OUT err            varchar(100),
   OUT member_id      int
)
SQL SECURITY DEFINER
BEGIN

  declare ck_exists int;
  set ck_exists = 1;
  set member_id = -1;
    
  main: begin

    select count(*) into ck_exists
    from members m
    where m.member_number = member_number;
    
    if (ck_exists > 0) then
      set err = "Member number already in database";
      leave main;
    end if;

    select count(*) into ck_exists
    from members m
    where m.handle = handle;
    
    if (ck_exists > 0) then
      set err = "handle already in database";
      leave main;
    end if;
      
    insert into members (member_number, name, handle, unlock_text) values (member_number, name, handle, unlock_text);
    set err = null;
    set member_id = last_insert_id();
    
  end main;

END //
DELIMITER ;


GRANT EXECUTE ON PROCEDURE sp_add_member TO 'gk'@'localhost'