drop procedure if exists sp_gg_set_manual_wc;

DELIMITER //
CREATE PROCEDURE sp_gg_set_manual_wc
(
  IN ggemail_id    int,
  IN ggsum_manual_wc int
)
SQL SECURITY DEFINER
BEGIN

  declare ck_exists int;
  declare ggemail_from varchar(200);
  declare ggemail_date timestamp;
  declare ggaddresses_id int;
  
  select count(*)
  into ck_exists
  from gg_summary ggs
  where ggs.ggemail_id = ggemail_id;
  
  if (ck_exists = 1) then
    update gg_summary
    set gg_summary.ggsum_manual_wc = ggsum_manual_wc
    where gg_summary.ggemail_id = ggemail_id;
  else
  
    select ggm.ggemail_from, ggm.ggemail_date
    into ggemail_from, ggemail_date
    from gg_email ggm
    where ggm.ggemail_id = ggemail_id;
  
    if (ggemail_date is not null) then    
      call sp_gg_get_address_id(ggemail_from, ggaddresses_id);
  
      insert into gg_summary (ggemail_id, ggaddresses_id, ggsum_manual_wc, ggemail_date)
      select ggemail_id, ggaddresses_id, ggsum_manual_wc, ggemail_date;
    end if;
  end if;

END //
DELIMITER ;

