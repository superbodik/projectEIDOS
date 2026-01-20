
print("[LUA] Script Engine Loaded. Welcome to the Matrix.")

function CalculateAttack(player, worldWeight)
    local might = player:get(Stat.Might)
    local luck = player:get(Stat.Luck)

    print("[LUA] Analyzing player stats... Might: " .. might)

    local multiplier = 1.0
    if luck > 50 then
        print("[LUA] Critical Hit Chance detected!")
        multiplier = 2.0
    end

    local damage = (might * (1 + worldWeight / 100.0)) * multiplier
    
    return damage
end