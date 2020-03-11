<?php

include "dbaccess_header.php";

$sql = <<<SQL
  select
    year(tu.start) as Year,
    monthname(tu.start) as Month,
    sec_to_time(sum(tu.duration)) as "Time (hh:mm:ss)",
    sec_to_time(SUM(CASE WHEN tu.tool_id=1 THEN tu.duration ELSE 0 END)) as "A0 (hh:mm:ss)",
    sec_to_time(SUM(CASE WHEN tu.tool_id=14 THEN tu.duration ELSE 0 END)) as "A2 (hh:mm:ss)",
    sec_to_time(SUM(CASE WHEN tu.status="CHARGED" THEN tu.duration ELSE 0 END)) as "Charged Time (hh:mm:ss)",
    FORMAT(SUM(CASE WHEN tu.status="CHARGED" THEN tu.duration ELSE 0 END)*(3/60/60), 2) as "Charged Income (£)",
    COUNT(DISTINCT(tu.user_id)) as "Distinct users",
    (
      select count(*)
      from role_updates ru
      inner join roles r on r.id = ru.added_role_id
      where r.name = 'tools.laser.user'
        and year(ru.created_at)=year(tu.start)
        and month(ru.created_at)=month(tu.start)
    ) as "Members inducted"
  from tool_usages tu
  where tu.tool_id in (1, 14)
    and tu.duration > 0
  group by year(tu.start),  monthname(tu.start)
  order by year(tu.start) desc,  month(tu.start) desc
SQL;

print sql_to_tt_json(db_pdo_link(), $sql);

?>
