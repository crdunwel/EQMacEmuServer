INSERT INTO blocked_spells (spellid, type, zoneid, x, y, z, x_diff, y_diff, z_diff, message, description)
SELECT
    (SELECT id FROM spells_new WHERE id = (SELECT clickeffect FROM items WHERE name = "Manastone" LIMIT 1) LIMIT 1) AS spell_id,
    1 AS type,
    zone.zoneidnumber AS zoneid,
    0 AS x,
    0 AS y,
    0 AS z,
    0 AS x_diff,
    0 AS y_diff,
    0 AS z_diff,
    '' AS message,
    'Manastone can only be used in non-plane classic zones' AS description
FROM zone
WHERE expansion != 1 OR long_name IN ('Plane of Hate', 'Plane of Sky', 'Plane of Fear');
