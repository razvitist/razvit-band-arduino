const form = document.querySelector('form');
form.addEventListener('submit', () => {
  setTimeout(() => document.location.reload(false), 500);
});
