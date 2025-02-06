// Track gambit packs with their values
let gambitPacks = [{
    id: 1,
    value: '999' // Default value from actions.json
}];

document.addEventListener('DOMContentLoaded', async () => {
    await populateDropdown('bnpcNameId', 'bnpcNameIds.json');
    await refreshGambitPacks();
    
    document.getElementById('bnpcNameId').addEventListener('change', updatePreview);
    
    document.getElementById('addGambitButton').addEventListener('click', async () => {
        gambitPacks.push({
            id: gambitPacks.length + 1,
            value: '999' // Default value from actions.json
        });
        await refreshGambitPacks();
        updatePreview();
    });

    document.getElementById('deleteGambitButton').addEventListener('click', async () => {
        if (gambitPacks.length > 1) {
            gambitPacks.pop();
            await refreshGambitPacks();
            updatePreview();
        }
    });
    
    updatePreview();
});

async function populateDropdown(elementId, fileName) {
    const selectElement = document.getElementById(elementId);
    try {
        selectElement.innerHTML = ''; // Clear existing options
        
        // Add default placeholder option
        const placeholder = document.createElement('option');
        placeholder.value = elementId === 'bnpcNameId' ? '9999' : '999';
        placeholder.textContent = elementId === 'bnpcNameId' ? 'Select BNPC...' : 'Select Action...';
        placeholder.selected = true;
        selectElement.appendChild(placeholder);

        const items = await window.electronAPI.getJsonData(fileName);
        items.forEach(item => {
            if ((item.nameid && item.nameid !== 9999) || (item.id && item.id !== 999)) {
                const option = document.createElement('option');
                option.value = item.nameid || item.id;
                option.textContent = item.name;
                selectElement.appendChild(option);
            }
        });
    } catch (error) {
        console.error(`Error populating ${elementId}:`, error);
    }
}

async function createGambitPackDropdown(packNumber, container) {
    const packContainer = document.createElement('div');
    packContainer.className = 'gambit-pack-container';
    packContainer.id = `pack-container-${packNumber}`;

    const label = document.createElement('div');
    label.className = 'gambit-pack-label';
    label.textContent = `Gambit Pack ${packNumber}:`;

    const select = document.createElement('select');
    select.id = `gambitPack-${packNumber}`;

    // Add placeholder first
    const placeholder = document.createElement('option');
    placeholder.value = '999';
    placeholder.textContent = 'Select Action...';
    select.appendChild(placeholder);

    // Populate dropdown
    const items = await window.electronAPI.getJsonData('actions.json');
    items.forEach(item => {
        if (item.id !== 999) {  // Skip the placeholder entry
            const option = document.createElement('option');
            option.value = item.id;
            option.textContent = item.name;
            select.appendChild(option);
        }
    });

    // Set the saved value for this pack
    select.value = gambitPacks[packNumber - 1].value;

    // Add change listener
    select.addEventListener('change', (e) => {
        gambitPacks[packNumber - 1].value = e.target.value;
        updatePreview();
    });

    // Add delete button
    const deleteButton = document.createElement('button');
    deleteButton.textContent = 'Delete';
    deleteButton.onclick = async () => {
        gambitPacks = gambitPacks.filter((_, index) => index !== packNumber - 1);
        await refreshGambitPacks();
        updatePreview();
    };

    packContainer.appendChild(label);
    packContainer.appendChild(select);
    packContainer.appendChild(deleteButton);
    container.appendChild(packContainer);
}
async function refreshGambitPacks() {
    const container = document.getElementById('gambitPacksContainer');
    container.innerHTML = ''; // Clear existing dropdowns
    
    for (let i = 0; i < gambitPacks.length; i++) {
        await createGambitPackDropdown(i + 1, container);
    }
}

async function getSelectedBnpcData(bnpcNameId) {
    const items = await window.electronAPI.getJsonData('bnpcNameIds.json');
    const selected = items.find(item => item.nameid.toString() === bnpcNameId.toString());
    return selected || { name: 'Default', basenameid: 55 };
}

function generateTemplateContent(bnpcNameId, bnpcData) {
    const className = `Bnpc${bnpcData.name}`;
    let gambitPacksContent = '';
    
    gambitPacks.forEach(pack => {
        gambitPacksContent += `        gambitPack->addTimeLine(
                Sapphire::World::AI::make_TopHateTargetCondition(),
                Sapphire::World::Action::make_Action( bnpc.getAsChara(), ${pack.value}, 0 ),
                0 );

`;
    });

    return `// File: src/scripts/battlenpc/${className}.cpp

#include <AI/GambitPack.h>
#include <AI/GambitTargetCondition.h>
#include <Action/Action.h>
#include <Actor/BNpc.h>
#include <Logging/Logger.h>
#include <ScriptObject.h>

using namespace Sapphire::World;
using namespace Sapphire::ScriptAPI;
using namespace Sapphire::World::AI;
using namespace Sapphire::Entity;

class ${className} : public BattleNpcScript
{
public:
    ${className}() : BattleNpcScript( ${bnpcData.basenameid} ) {}

    void onInit( Sapphire::Entity::BNpc& bnpc ) override
    {
        bnpc.setBNpcNameId( ${bnpcNameId} );
        auto gambitPack = std::make_shared< Sapphire::World::AI::GambitTimeLinePack >( -1 );

${gambitPacksContent}        m_gambitPack = gambitPack;
        bnpc.setGambitPack( m_gambitPack );
    }

private:
    std::shared_ptr< Sapphire::World::AI::GambitPack > m_gambitPack;
};

EXPOSE_SCRIPT( ${className} );`;
}

async function updatePreview() {
    const bnpcNameId = document.getElementById('bnpcNameId').value;
    const bnpcData = await getSelectedBnpcData(bnpcNameId);
    const previewElement = document.getElementById('preview');
    
    // Only show preview if a valid BNPC is selected
    if (bnpcNameId && bnpcNameId !== '9999') {
        const content = generateTemplateContent(bnpcNameId, bnpcData);
        previewElement.textContent = content;
    } else {
        previewElement.textContent = '// Please select a BNPC...';
    }
}

document.getElementById('generateButton').addEventListener('click', async () => {
    const bnpcNameId = document.getElementById('bnpcNameId').value;
    if (!bnpcNameId || bnpcNameId === '9999') {
        alert('Please select a BNPC first');
        return;
    }
    
    const bnpcData = await getSelectedBnpcData(bnpcNameId);
    const content = generateTemplateContent(bnpcNameId, bnpcData);

    const success = await window.electronAPI.saveScript(content);
    if (success) {
        alert('Script generated successfully!');
    } else {
        alert('Error generating script');
    }
});