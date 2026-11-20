// JUIT Club Hub front end. Plain JavaScript only, no libraries.

var app = document.getElementById('app');
var selectedSkills = [];
var studentName = '';

// Escapes text from the API before it goes into HTML.
function esc(text) {
  return String(text === undefined || text === null ? '' : text)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;');
}

// One fetch call that returns JSON.
function getJson(url) {
  return fetch(url).then(function (res) {
    if (!res.ok) {
      throw new Error('Request failed with ' + res.status);
    }
    return res.json();
  });
}

// Friendly screen shown when the C++ server is not running.
function serverDown() {
  app.innerHTML =
    '<section class="page"><div class="panel">' +
    '<p class="kicker">Server offline</p>' +
    '<h1>Start the server first</h1>' +
    '<p class="note">This site needs the small C++ server that sits in the same folder. ' +
    'Open a terminal in that folder and run:</p>' +
    '<pre class="code">g++ server.cpp -o server -std=c++11\n./server</pre>' +
    '<p class="note">On Windows run <span class="mono">g++ server.cpp -o server -std=c++11 -lws2_32</span> ' +
    'and then <span class="mono">server.exe</span>. Then open ' +
    '<span class="mono">http://localhost:8080</span>.</p>' +
    '</div></section>';
}

// Marks the current nav link.
function setActiveNav(name) {
  var links = document.querySelectorAll('.site-nav a');
  for (var i = 0; i < links.length; i++) {
    if (links[i].getAttribute('data-route') === name) {
      links[i].className = 'active';
    } else {
      links[i].className = '';
    }
  }
}

// Fades in .reveal elements when they scroll into view.
function setupReveal() {
  var items = document.querySelectorAll('.reveal');
  if (!('IntersectionObserver' in window)) {
    for (var i = 0; i < items.length; i++) items[i].className += ' on';
    return;
  }
  var watcher = new IntersectionObserver(function (entries) {
    for (var k = 0; k < entries.length; k++) {
      if (entries[k].isIntersecting) {
        entries[k].target.className += ' on';
        watcher.unobserve(entries[k].target);
      }
    }
  }, { threshold: 0.15 });
  for (var j = 0; j < items.length; j++) watcher.observe(items[j]);
}

// One word status label, with a live dot for running events.
function statusMark(ev) {
  if (ev.status === 'ongoing') return '<span class="dot live"></span> Live';
  if (ev.status === 'upcoming') return 'Upcoming';
  return 'Completed';
}

// Facts block for one event.
function factsHtml(ev) {
  var out = '<div class="ev-meta">';
  out += '<div><span class="label">Hosts</span> ' + esc(ev.hosts.join(' and ')) + '</div>';
  out += '<div><span class="label">Venue</span> ' + esc(ev.venue) + '</div>';
  out += '<div><span class="label">Dates</span> ' + esc(ev.startDate) + ' to ' + esc(ev.endDate) + '</div>';
  out += '<div><span class="label">Skill needed</span> ' + esc(ev.skill) + '</div>';
  out += '<div><span class="label">Q and A</span> ' + esc(ev.contact.name) + ' | ' +
    esc(ev.contact.phone) + ' | ' + esc(ev.contact.email) + '</div>';
  out += '</div>';
  return out;
}

// Stalls block for one event, with menus and prices.
function stallsHtml(ev) {
  if (!ev.stalls || ev.stalls.length === 0) return '';
  var out = '<div class="stalls"><h4>Stalls</h4>';
  for (var i = 0; i < ev.stalls.length; i++) {
    var st = ev.stalls[i];
    out += '<div class="stall"><h5>' + esc(st.name) + '</h5>';
    out += '<p class="meta">' + esc(st.location) + ' | ' + esc(st.timing) + '</p>';
    out += '<ul class="menu">';
    for (var m = 0; m < st.menu.length; m++) {
      out += '<li><span>' + esc(st.menu[m].item) + '</span>' +
        '<span class="dots"></span><span>Rs ' + esc(st.menu[m].price) + '</span></li>';
    }
    out += '</ul></div>';
  }
  return out + '</div>';
}

// Sponsors block for one event.
function sponsorsHtml(ev) {
  if (!ev.sponsors || ev.sponsors.length === 0) return '';
  var out = '<div class="ev-sponsors"><h4>Sponsors</h4>';
  for (var i = 0; i < ev.sponsors.length; i++) {
    var sp = ev.sponsors[i];
    out += '<a class="sponsor-line" href="' + esc(sp.website) + '" target="_blank" rel="noopener">' +
      '<strong>' + esc(sp.name) + '</strong> ' + esc(sp.info) + '</a>';
  }
  return out + '</div>';
}

// Slowly scrolling sponsor strip, list repeated twice.
function sponsorStrip(list) {
  if (list.length === 0) return '';
  var inner = '';
  for (var i = 0; i < list.length; i++) {
    inner += '<a href="' + esc(list[i].website) + '" target="_blank" rel="noopener">' +
      esc(list[i].name) + '</a><span class="sep">/</span>';
  }
  return '<div class="strip" aria-label="Sponsors">' +
    '<div class="strip-track">' +
    '<div class="strip-half">' + inner + '</div>' +
    '<div class="strip-half">' + inner + '</div>' +
    '</div></div>';
}

// Loads the home page data.
function loadHome() {
  app.innerHTML = '<section class="page"><p class="note">Loading the home page...</p></section>';
  Promise.all([getJson('/api/home'), getJson('/api/posts'), getJson('/api/sponsors')])
    .then(function (data) { renderHome(data[0], data[1], data[2]); })
    .catch(serverDown);
}

// Draws the home page.
function renderHome(home, posts, sponsors) {
  var list = home.events || [];
  var live = null;
  var rest = [];
  for (var i = 0; i < list.length; i++) {
    if (list[i].status === 'ongoing' && live === null) live = list[i];
    else rest.push(list[i]);
  }
  if (live === null && list.length > 0) {
    live = list[0];
    rest = list.slice(1);
  }

  var hero = '';
  if (live) {
    hero = '<section class="hero" aria-label="Event happening now">' +
      '<div class="hero-text">' +
      '<p class="kicker"><span class="dot live"></span> Happening now</p>' +
      '<h1>' + esc(live.name) + '</h1>' +
      '<p class="lede">' + esc(live.description) + '</p>' +
      '<p class="hero-meta">' + esc(live.hosts.join(' and ')) + ' at ' + esc(live.venue) +
      ' from ' + esc(live.startDate) + ' to ' + esc(live.endDate) + '</p>' +
      '</div>' +
      '<div class="hero-side">' + factsHtml(live) + stallsHtml(live) + sponsorsHtml(live) + '</div>' +
      '</section>';
  }

  var rows = '';
  for (var r = 0; r < rest.length; r++) {
    var ev = rest[r];
    rows += '<article class="ev-row reveal">' +
      '<div class="ev-date">' + esc(ev.startDate) +
      '<br><span class="meta">' + statusMark(ev) + '</span></div>' +
      '<div class="ev-body"><h3>' + esc(ev.name) + '</h3>' +
      factsHtml(ev) + stallsHtml(ev) + sponsorsHtml(ev) + '</div>' +
      '</article>';
  }
  if (rows === '') rows = '<p class="note">No other events are listed right now.</p>';

  var postCards = '';
  var postList = posts.posts || [];
  for (var p = 0; p < postList.length; p++) {
    postCards += '<a class="post reveal" href="https://www.instagram.com/' +
      esc(postList[p].handle) + '" target="_blank" rel="noopener">' +
      '<p class="post-handle">@' + esc(postList[p].handle) + '</p>' +
      '<p class="post-caption">' + esc(postList[p].caption) + '</p>' +
      '<p class="meta">' + esc(postList[p].date) + '</p></a>';
  }

  app.innerHTML = '<section class="page">' + hero +
    '<div class="section-head"><h2>Coming up next</h2>' +
    '<span class="meta">Upcoming events</span></div>' +
    '<div class="ev-list">' + rows + '</div>' +
    '<div class="section-head"><h2>From the clubs</h2>' +
    '<span class="meta">Sample instagram posts</span></div>' +
    '<div class="posts">' + postCards + '</div>' +
    sponsorStrip(sponsors.sponsors || []) +
    '</section>';
  setupReveal();
}

// Loads the upcoming or ongoing event list.
function loadEvents(status) {
  app.innerHTML = '<section class="page"><p class="note">Loading events...</p></section>';
  getJson('/api/events?status=' + status).then(function (data) {
    var list = data.events || [];
    var title = status === 'ongoing' ? 'Ongoing events' : 'Upcoming events';
    var rows = '';
    for (var i = 0; i < list.length; i++) {
      var ev = list[i];
      rows += '<article class="ev-row reveal">' +
        '<div class="ev-date">' + esc(ev.startDate) +
        '<br><span class="meta">' + statusMark(ev) + '</span></div>' +
        '<div class="ev-body"><h3>' + esc(ev.name) + '</h3>' +
        factsHtml(ev) + stallsHtml(ev) + sponsorsHtml(ev) + '</div>' +
        '</article>';
    }
    if (rows === '') rows = '<p class="note">Nothing is listed here right now.</p>';
    app.innerHTML = '<section class="page">' +
      '<header class="page-head"><p class="kicker">Events</p><h1>' + title + '</h1>' +
      '<p class="lede">Sorted by the date the event starts.</p></header>' +
      '<div class="ev-list">' + rows + '</div></section>';
    setupReveal();
  }).catch(serverDown);
}

// Loads the skills list and the first audition result.
function loadAuditions() {
  selectedSkills = [];
  app.innerHTML = '<section class="page"><p class="note">Loading auditions...</p></section>';
  Promise.all([getJson('/api/skills'), getJson('/api/auditions')])
    .then(function (data) { drawAuditions(data[0].skills || [], data[1]); })
    .catch(serverDown);
}

// Draws the audition page with the sticky filter bar.
function drawAuditions(skills, result) {
  var chips = '';
  for (var i = 0; i < skills.length; i++) {
    var on = selectedSkills.indexOf(skills[i]) >= 0;
    chips += '<button type="button" class="chip' + (on ? ' on' : '') +
      '" data-skill="' + esc(skills[i]) + '" aria-pressed="' + on + '">' +
      esc(skills[i]) + '</button>';
  }
  app.innerHTML = '<section class="page">' +
    '<header class="page-head"><p class="kicker">Auditions</p>' +
    '<h1>Find a club that needs your skills</h1>' +
    '<p class="lede">Tick your skills. Clubs that need more of them are listed first.</p></header>' +
    '<div class="filter-bar"><div class="filter-inner">' +
    '<div class="chips" role="group" aria-label="Skill filter">' + chips + '</div>' +
    '<label class="name-field">Your name <input id="studentName" type="text" autocomplete="name" /></label>' +
    '</div></div>' +
    '<p id="recent" class="recent"></p>' +
    '<div id="results" class="results"></div>' +
    '<p id="interestStatus" class="status" role="status"></p>' +
    '</section>';

  var nameInput = document.getElementById('studentName');
  nameInput.value = studentName;
  nameInput.addEventListener('input', function () { studentName = this.value; });

  bindChips();
  showAuditionResults(result);
}

// Wires up the skill chips.
function bindChips() {
  var chips = document.querySelectorAll('.chip');
  for (var i = 0; i < chips.length; i++) {
    chips[i].addEventListener('click', function () {
      var skill = this.getAttribute('data-skill');
      var at = selectedSkills.indexOf(skill);
      if (at >= 0) selectedSkills.splice(at, 1);
      else selectedSkills.push(skill);
      var on = at < 0;
      this.className = on ? 'chip on' : 'chip';
      this.setAttribute('aria-pressed', on ? 'true' : 'false');
      askAuditions();
    });
  }
}

// Asks the server for ranked clubs for the chosen skills.
function askAuditions() {
  getJson('/api/auditions?skills=' + encodeURIComponent(selectedSkills.join(',')))
    .then(showAuditionResults)
    .catch(serverDown);
}

// Draws the ranked club cards and the recent searches.
function showAuditionResults(result) {
  var recentBox = document.getElementById('recent');
  var recent = result.recent || [];
  recentBox.textContent = recent.length > 0 ? 'Recent searches: ' + recent.join(', ') : '';

  var box = document.getElementById('results');
  var clubs = result.clubs || [];
  if (clubs.length === 0) {
    box.innerHTML = '<div class="panel"><h2>No club matches these skills yet</h2>' +
      '<p class="note">Try fewer chips, a different skill, or clear all chips to see every open audition.</p></div>';
    return;
  }
  var html = '';
  for (var i = 0; i < clubs.length; i++) {
    var c = clubs[i];
    var tag = c.matchCount > 0
      ? c.matchCount + (c.matchCount === 1 ? ' skill match' : ' skills match')
      : 'Open audition';
    html += '<article class="aud-card stagger" style="animation-delay:' + (i * 60) + 'ms">' +
      '<div class="aud-top"><h3>' + esc(c.name) + '</h3>' +
      '<span class="tag">' + esc(tag) + '</span></div>' +
      '<p class="meta">' + esc(c.group) + ' group | Audition venue ' + esc(c.venue) + '</p>' +
      '<p><span class="label">Skills needed</span> ' + esc((c.skills || []).join(', ')) + '</p>' +
      '<p><span class="label">When</span> ' + esc(c.date) + ' at ' + esc(c.time) + '</p>' +
      '<p><span class="label">Contact</span> ' + esc(c.contactName) + ' | ' +
      esc(c.phone) + ' | ' + esc(c.email) + '</p>' +
      '<button type="button" class="btn interest" data-club="' + esc(c.name) +
      '">I am interested</button>' +
      '</article>';
  }
  box.innerHTML = html;
  bindInterest();
}

// Wires up the interest buttons.
function bindInterest() {
  var buttons = document.querySelectorAll('.interest');
  for (var i = 0; i < buttons.length; i++) {
    buttons[i].addEventListener('click', function () {
      var club = this.getAttribute('data-club');
      var name = studentName.trim();
      if (name === '') {
        document.getElementById('interestStatus').textContent =
          'Write your name in the box above the results first.';
        return;
      }
      sendInterest(club, name);
    });
  }
}

// Sends one interest request to the server.
function sendInterest(club, name) {
  var status = document.getElementById('interestStatus');
  status.textContent = 'Sending...';
  fetch('/api/interest', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: 'club=' + encodeURIComponent(club) + '&student=' + encodeURIComponent(name)
  }).then(function (res) {
    if (!res.ok) throw new Error('Request failed');
    return res.json();
  }).then(function (data) {
    status.textContent = data.message;
  }).catch(serverDown);
}

// Loads every club and splits them into the two groups.
function loadClubs() {
  app.innerHTML = '<section class="page"><p class="note">Loading clubs...</p></section>';
  getJson('/api/clubs').then(function (data) {
    var jyc = '';
    var tech = '';
    var list = data.clubs || [];
    for (var i = 0; i < list.length; i++) {
      var c = list[i];
      var row = '<article class="club-row reveal">' +
        '<div class="club-head"><h3>' + esc(c.name) + '</h3>' +
        '<span class="tag' + (c.auditionOpen ? ' open' : '') + '">' +
        (c.auditionOpen ? 'Auditions open' : 'Auditions closed') + '</span></div>' +
        '<p class="desc">' + esc(c.description) + '</p>' +
        '<p class="meta"><span class="label">Skills</span> ' +
        esc((c.skills || []).join(', ')) + '</p>' +
        '<p class="meta"><span class="label">Contact</span> ' + esc(c.contactName) +
        ' | ' + esc(c.phone) + ' | ' + esc(c.email) + '</p>' +
        '<a class="ig" href="https://www.instagram.com/' + esc(c.handle) +
        '" target="_blank" rel="noopener">@' + esc(c.handle) + '</a>' +
        '</article>';
      if (c.group === 'JYC') jyc += row;
      else tech += row;
    }
    app.innerHTML = '<section class="page">' +
      '<header class="page-head"><p class="kicker">Clubs</p><h1>Every club on campus</h1>' +
      '<p class="lede">JYC clubs and the technical clubs and student branches.</p></header>' +
      '<div class="ledger">' +
      '<div class="ledger-col"><h2>JYC clubs</h2>' + jyc + '</div>' +
      '<div class="ledger-col"><h2>Technical and other</h2>' + tech + '</div>' +
      '</div></section>';
    setupReveal();
  }).catch(serverDown);
}

// Opens the page named in the URL hash.
function route() {
  var name = window.location.hash.replace('#', '');
  if (name === 'app') return;
  if (name === '') name = 'home';
  setActiveNav(name);
  window.scrollTo(0, 0);
  if (name === 'upcoming') loadEvents('upcoming');
  else if (name === 'ongoing') loadEvents('ongoing');
  else if (name === 'auditions') loadAuditions();
  else if (name === 'clubs') loadClubs();
  else loadHome();
}

window.addEventListener('hashchange', route);
route();