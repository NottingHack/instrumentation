<?php

include "dbaccess_header.php";

$sql = <<<SQL
  SELECT
    COUNT(DISTINCT stats.last_day) AS last_day,
    COUNT(DISTINCT stats.last_week) AS last_week,
    COUNT(DISTINCT stats.last_month) AS last_month,
    COUNT(DISTINCT stats.last_quarter) AS last_quarter,
    COUNT(DISTINCT stats.last_year) AS last_year,
    COUNT(DISTINCT stats.anytime) AS anytime,
    (SELECT COUNT(*) FROM user u LEFT JOIN role_user ru ON u.id = ru.user_id LEFT JOIN roles r ON ru.role_id = r.id WHERE r.name = 'member.current') AS total_current_members

  FROM (

  -- Current members accessing the space
  SELECT
    IF(al.access_time >= DATE_SUB(NOW(), INTERVAL 1 DAY), u.id, NULL) AS last_day,
    IF(al.access_time >= DATE_SUB(NOW(), INTERVAL 1 WEEK), u.id, NULL) AS last_week,
    IF(al.access_time >= DATE_SUB(NOW(), INTERVAL 1 MONTH), u.id, NULL) AS last_month,
    IF(al.access_time >= DATE_SUB(NOW(), INTERVAL 1 QUARTER), u.id, NULL) AS last_quarter,
    IF(al.access_time >= DATE_SUB(NOW(), INTERVAL 1 YEAR), u.id, NULL) AS last_year,
    u.id AS anytime

  FROM
    user u
    LEFT JOIN role_user ru ON u.id = ru.user_id LEFT JOIN roles r ON ru.role_id = r.id
    JOIN access_logs al ON u.id = al.user_id
    JOIN
    (
      SELECT
        al.user_id,
        MAX(al.access_time) max_access_time
      FROM
        access_logs al
      GROUP BY
        al.user_id
    ) max_al ON
    al.user_id = max_al.user_id
    AND
    al.access_time = max_al.max_access_time

  WHERE
    r.name = 'member.current'

  UNION

  -- Current members using an access controlled tool
  SELECT
    IF(tool_usages.start >= DATE_SUB(NOW(), INTERVAL 1 DAY), u.id, NULL) AS last_day,
    IF(tool_usages.start >= DATE_SUB(NOW(), INTERVAL 1 WEEK), u.id, NULL) AS last_week,
    IF(tool_usages.start >= DATE_SUB(NOW(), INTERVAL 1 MONTH), u.id, NULL) AS last_month,
    IF(tool_usages.start >= DATE_SUB(NOW(), INTERVAL 1 QUARTER), u.id, NULL) AS last_quarter,
    IF(tool_usages.start >= DATE_SUB(NOW(), INTERVAL 1 YEAR), u.id, NULL) AS last_year,
    u.id AS anytime

  FROM
    user u
    LEFT JOIN role_user ru ON u.id = ru.user_id LEFT JOIN roles r ON ru.role_id = r.id
    JOIN tool_usages ON u.id = tool_usages.user_id
    JOIN
    (
      SELECT
        tool_usages.user_id,
        MAX(tool_usages.start) max_start
      FROM
        tool_usages
      GROUP BY
        tool_usages.user_id
    ) max_tool_usages ON
    tool_usages.user_id = max_tool_usages.user_id
    AND
    tool_usages.start = max_tool_usages.max_start

  WHERE
    r.name = 'member.current'

  UNION

  -- Current members using the vending machine or note acceptor
  SELECT
    IF(vend_logs.enqueued_time >= DATE_SUB(NOW(), INTERVAL 1 DAY), u.id, NULL) AS last_day,
    IF(vend_logs.enqueued_time >= DATE_SUB(NOW(), INTERVAL 1 WEEK), u.id, NULL) AS last_week,
    IF(vend_logs.enqueued_time >= DATE_SUB(NOW(), INTERVAL 1 MONTH), u.id, NULL) AS last_month,
    IF(vend_logs.enqueued_time >= DATE_SUB(NOW(), INTERVAL 1 QUARTER), u.id, NULL) AS last_quarter,
    IF(vend_logs.enqueued_time >= DATE_SUB(NOW(), INTERVAL 1 YEAR), u.id, NULL) AS last_year,
    u.id AS anytime

  FROM
    user u
    LEFT JOIN role_user ru ON u.id = ru.user_id LEFT JOIN roles r ON ru.role_id = r.id
    JOIN vend_logs ON u.id = vend_logs.user_id
    JOIN
    (
      SELECT
        vend_logs.user_id,
        MAX(vend_logs.enqueued_time) max_enqueued_time
      FROM
        vend_logs
      GROUP BY
        vend_logs.user_id
    ) max_vend_logs ON
    vend_logs.user_id = max_vend_logs.user_id
    AND
    vend_logs.enqueued_time = max_vend_logs.max_enqueued_time

  WHERE
    r.name = 'member.current'

  ) stats;
SQL;

print sql_to_json(db_pdo_link(), $sql);

?>